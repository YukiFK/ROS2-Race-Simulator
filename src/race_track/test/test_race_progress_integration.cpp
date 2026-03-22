#include <gtest/gtest.h>

#include <ament_index_cpp/get_package_prefix.hpp>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "race_interfaces/msg/lap_event.hpp"
#include "race_interfaces/msg/race_command.hpp"
#include "race_interfaces/msg/race_state.hpp"
#include "race_interfaces/msg/vehicle_race_status.hpp"
#include "rclcpp/rclcpp.hpp"

namespace race_track
{
namespace
{

using namespace std::chrono_literals;

constexpr auto kCommandSubscriberTimeout = 5s;
constexpr auto kTwoVehicleStatusTimeout = 6s;
constexpr auto kLapEventTimeout = 12s;
constexpr auto kSingleFinisherTimeout = 12s;
constexpr auto kRaceCompletionTimeout = 16s;
constexpr auto kStopTimeout = 3s;
constexpr auto kResetRestartTimeout = 6s;
constexpr auto kQuietWindowAfterStop = 1500ms;
constexpr auto kSpinPollInterval = 50ms;
constexpr auto kPostCommandSettleDelay = 200ms;
constexpr char kPublisherExecutable[] = "race_progress_publisher";
constexpr char kVehicleOneId[] = "demo_vehicle_1";
constexpr char kVehicleTwoId[] = "demo_vehicle_2";

std::string makeIsolatedRosDomainId()
{
  const long pid = static_cast<long>(::getpid());
  return std::to_string(100 + (pid % 101));
}

std::filesystem::path resolvePublisherExecutablePath()
{
  const std::filesystem::path package_prefix =
    ament_index_cpp::get_package_prefix("race_track");
  return package_prefix / "lib" / "race_track" / kPublisherExecutable;
}

class ScopedPublisherProcess
{
public:
  ScopedPublisherProcess()
  {
    const std::filesystem::path executable_path = resolvePublisherExecutablePath();
    if (!std::filesystem::exists(executable_path)) {
      throw std::runtime_error("Failed to locate race_progress_publisher executable");
    }

    child_pid_ = ::fork();
    if (child_pid_ < 0) {
      throw std::runtime_error(std::string("fork failed: ") + std::strerror(errno));
    }

    if (child_pid_ == 0) {
      const std::string executable_string = executable_path.string();
      std::vector<char> target_lap_count_arg = {'t', 'a', 'r', 'g', 'e', 't', '_', 'l', 'a', 'p', '_',
        'c', 'o', 'u', 'n', 't', ':', '=', '2', '\0'};
      char * const argv[] = {
        const_cast<char *>(executable_string.c_str()),
        const_cast<char *>("--ros-args"),
        const_cast<char *>("-p"),
        target_lap_count_arg.data(),
        nullptr,
      };
      ::execv(executable_string.c_str(), argv);
      std::perror("execv");
      _exit(1);
    }
  }

  ScopedPublisherProcess(const ScopedPublisherProcess &) = delete;
  ScopedPublisherProcess & operator=(const ScopedPublisherProcess &) = delete;

  ~ScopedPublisherProcess()
  {
    terminate();
  }

private:
  void terminate()
  {
    if (child_pid_ <= 0) {
      return;
    }

    ::kill(child_pid_, SIGINT);

    for (int attempt = 0; attempt < 20; ++attempt) {
      int status = 0;
      const pid_t wait_result = ::waitpid(child_pid_, &status, WNOHANG);
      if (wait_result == child_pid_) {
        child_pid_ = -1;
        return;
      }
      std::this_thread::sleep_for(100ms);
    }

    ::kill(child_pid_, SIGKILL);
    int status = 0;
    static_cast<void>(::waitpid(child_pid_, &status, 0));
    child_pid_ = -1;
  }

  pid_t child_pid_{-1};
};

class IntegrationObserver : public rclcpp::Node
{
public:
  IntegrationObserver()
  : Node("race_progress_integration_test")
  {
    race_state_subscription_ = create_subscription<race_interfaces::msg::RaceState>(
      "/race_state", 10,
      std::bind(&IntegrationObserver::onRaceState, this, std::placeholders::_1));
    vehicle_status_subscription_ =
      create_subscription<race_interfaces::msg::VehicleRaceStatus>(
      "/vehicle_race_status", 10,
      std::bind(&IntegrationObserver::onVehicleRaceStatus, this, std::placeholders::_1));
    lap_event_subscription_ = create_subscription<race_interfaces::msg::LapEvent>(
      "/lap_event", 10,
      std::bind(&IntegrationObserver::onLapEvent, this, std::placeholders::_1));
    command_publisher_ = create_publisher<race_interfaces::msg::RaceCommand>("/race_command", 10);
  }

  bool commandSubscriberConnected() const
  {
    return command_publisher_->get_subscription_count() > 0U;
  }

  void publishCommand(const std::uint8_t command)
  {
    race_interfaces::msg::RaceCommand message;
    message.header.stamp = get_clock()->now();
    message.header.frame_id = "map";
    message.command = command;
    command_publisher_->publish(message);
  }

  std::size_t totalVehicleStatusCount() const
  {
    std::scoped_lock lock(mutex_);
    return total_vehicle_status_count_;
  }

  std::size_t latestVehicleStatusCount(const std::string & vehicle_id) const
  {
    std::scoped_lock lock(mutex_);
    const auto iterator = latest_vehicle_statuses_.find(vehicle_id);
    return iterator != latest_vehicle_statuses_.end() ? iterator->second.sequence : 0U;
  }

  bool hasVehicleStatusesForBothVehicles() const
  {
    std::scoped_lock lock(mutex_);
    return latest_vehicle_statuses_.count(kVehicleOneId) > 0U &&
      latest_vehicle_statuses_.count(kVehicleTwoId) > 0U;
  }

  bool hasLapEventsForBothVehicles() const
  {
    std::scoped_lock lock(mutex_);
    return observed_lap_event_vehicle_ids_.count(kVehicleOneId) > 0U &&
      observed_lap_event_vehicle_ids_.count(kVehicleTwoId) > 0U;
  }

  bool observedRunningWhileOnlyOneVehicleFinished() const
  {
    std::scoped_lock lock(mutex_);
    return observed_single_vehicle_finished_before_completion_;
  }

  bool raceCompletedAfterBothVehiclesFinished() const
  {
    std::scoped_lock lock(mutex_);
    return latest_race_status_ == "completed" && finished_vehicle_ids_.size() == 2U;
  }

  bool raceStopped() const
  {
    std::scoped_lock lock(mutex_);
    return latest_race_status_ == "stopped";
  }

  bool restartedWithInitialVehicleStatuses(
    const std::size_t vehicle_one_count_before_restart,
    const std::size_t vehicle_two_count_before_restart) const
  {
    std::scoped_lock lock(mutex_);
    return hasFreshInitialStatusLocked(kVehicleOneId, vehicle_one_count_before_restart) &&
      hasFreshInitialStatusLocked(kVehicleTwoId, vehicle_two_count_before_restart);
  }

private:
  struct SequencedVehicleStatus
  {
    race_interfaces::msg::VehicleRaceStatus message;
    std::size_t sequence{0U};
  };

  bool hasFreshInitialStatusLocked(
    const std::string & vehicle_id, const std::size_t sequence_before_restart) const
  {
    const auto iterator = latest_vehicle_statuses_.find(vehicle_id);
    if (iterator == latest_vehicle_statuses_.end()) {
      return false;
    }

    const auto & latest_status = iterator->second;
    return latest_status.sequence > sequence_before_restart &&
      latest_status.message.lap_count == 0 &&
      !latest_status.message.has_finished;
  }

  void onRaceState(const race_interfaces::msg::RaceState::SharedPtr msg)
  {
    std::scoped_lock lock(mutex_);
    latest_race_status_ = msg->race_status;
  }

  void onVehicleRaceStatus(const race_interfaces::msg::VehicleRaceStatus::SharedPtr msg)
  {
    std::scoped_lock lock(mutex_);
    ++total_vehicle_status_count_;
    latest_vehicle_statuses_[msg->vehicle_id] = SequencedVehicleStatus{*msg, total_vehicle_status_count_};
    if (msg->has_finished) {
      finished_vehicle_ids_.insert(msg->vehicle_id);
      if (finished_vehicle_ids_.size() == 1U && latest_race_status_ != "completed") {
        observed_single_vehicle_finished_before_completion_ = true;
      }
    }
  }

  void onLapEvent(const race_interfaces::msg::LapEvent::SharedPtr msg)
  {
    std::scoped_lock lock(mutex_);
    observed_lap_event_vehicle_ids_.insert(msg->vehicle_id);
  }

  mutable std::mutex mutex_;
  std::string latest_race_status_;
  std::map<std::string, SequencedVehicleStatus> latest_vehicle_statuses_;
  std::set<std::string> observed_lap_event_vehicle_ids_;
  std::set<std::string> finished_vehicle_ids_;
  bool observed_single_vehicle_finished_before_completion_{false};
  std::size_t total_vehicle_status_count_{0U};
  rclcpp::Subscription<race_interfaces::msg::RaceState>::SharedPtr race_state_subscription_;
  rclcpp::Subscription<race_interfaces::msg::VehicleRaceStatus>::SharedPtr
    vehicle_status_subscription_;
  rclcpp::Subscription<race_interfaces::msg::LapEvent>::SharedPtr lap_event_subscription_;
  rclcpp::Publisher<race_interfaces::msg::RaceCommand>::SharedPtr command_publisher_;
};

class RaceProgressIntegrationTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    const std::string ros_domain_id = makeIsolatedRosDomainId();
    ::setenv("ROS_DOMAIN_ID", ros_domain_id.c_str(), 1);
    const std::filesystem::path ros_log_dir = std::filesystem::path("/tmp") / "ros_logs";
    std::filesystem::create_directories(ros_log_dir);
    ::setenv("ROS_LOG_DIR", ros_log_dir.c_str(), 1);

    if (!rclcpp::ok()) {
      int argc = 0;
      char ** argv = nullptr;
      rclcpp::init(argc, argv);
    }
  }

  static void TearDownTestSuite()
  {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }

  void SetUp() override
  {
    publisher_process_ = std::make_unique<ScopedPublisherProcess>();
    observer_ = std::make_shared<IntegrationObserver>();
    executor_.add_node(observer_);

    ASSERT_TRUE(spinUntil(
      [this]() {
        return observer_->commandSubscriberConnected();
      },
      kCommandSubscriberTimeout));
  }

  void TearDown() override
  {
    if (observer_) {
      executor_.remove_node(observer_);
      observer_.reset();
    }
    publisher_process_.reset();
  }

  template<typename PredicateT>
  bool spinUntil(PredicateT predicate, const std::chrono::steady_clock::duration timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      executor_.spin_some();
      if (predicate()) {
        return true;
      }
      std::this_thread::sleep_for(kSpinPollInterval);
    }

    executor_.spin_some();
    return predicate();
  }

  void spinFor(const std::chrono::steady_clock::duration duration)
  {
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < deadline) {
      executor_.spin_some();
      std::this_thread::sleep_for(kSpinPollInterval);
    }
    executor_.spin_some();
  }

  void publishCommandAndAllowDelivery(const std::uint8_t command)
  {
    observer_->publishCommand(command);
    spinFor(kPostCommandSettleDelay);
  }

  rclcpp::executors::SingleThreadedExecutor executor_;
  std::shared_ptr<IntegrationObserver> observer_;
  std::unique_ptr<ScopedPublisherProcess> publisher_process_;
};

TEST_F(RaceProgressIntegrationTest, PublishesTwoVehicleProgressAndCompletesOnlyAfterBothFinish)
{
  publishCommandAndAllowDelivery(race_interfaces::msg::RaceCommand::START);

  EXPECT_TRUE(spinUntil(
    [this]() {
      return observer_->hasVehicleStatusesForBothVehicles();
    },
    kTwoVehicleStatusTimeout));

  EXPECT_TRUE(spinUntil(
    [this]() {
      return observer_->hasLapEventsForBothVehicles();
    },
    kLapEventTimeout));

  EXPECT_TRUE(spinUntil(
    [this]() {
      return observer_->observedRunningWhileOnlyOneVehicleFinished();
    },
    kSingleFinisherTimeout));

  EXPECT_TRUE(spinUntil(
    [this]() {
      return observer_->raceCompletedAfterBothVehiclesFinished();
    },
    kRaceCompletionTimeout));
}

TEST_F(RaceProgressIntegrationTest, StopHaltsRaceWideProgressAndResetRestartsBothVehiclesCleanly)
{
  publishCommandAndAllowDelivery(race_interfaces::msg::RaceCommand::START);

  ASSERT_TRUE(spinUntil(
    [this]() {
      return observer_->hasVehicleStatusesForBothVehicles();
    },
    kTwoVehicleStatusTimeout));

  publishCommandAndAllowDelivery(race_interfaces::msg::RaceCommand::STOP);

  ASSERT_TRUE(spinUntil(
    [this]() {
      return observer_->raceStopped();
    },
    kStopTimeout));

  const std::size_t status_count_after_stop = observer_->totalVehicleStatusCount();
  EXPECT_FALSE(spinUntil(
    [this, status_count_after_stop]() {
      return observer_->totalVehicleStatusCount() > status_count_after_stop;
    },
    kQuietWindowAfterStop));

  publishCommandAndAllowDelivery(race_interfaces::msg::RaceCommand::RESET);

  ASSERT_TRUE(spinUntil(
    [this]() {
      return observer_->raceStopped();
    },
    kStopTimeout));

  const std::size_t vehicle_one_count_before_restart = observer_->latestVehicleStatusCount(kVehicleOneId);
  const std::size_t vehicle_two_count_before_restart = observer_->latestVehicleStatusCount(kVehicleTwoId);

  publishCommandAndAllowDelivery(race_interfaces::msg::RaceCommand::START);

  EXPECT_TRUE(spinUntil(
    [this, vehicle_one_count_before_restart, vehicle_two_count_before_restart]() {
      return observer_->restartedWithInitialVehicleStatuses(
        vehicle_one_count_before_restart, vehicle_two_count_before_restart);
    },
    kResetRestartTimeout));
}

}  // namespace
}  // namespace race_track
