#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>

#include "race_interfaces/msg/race_command.hpp"
#include "rclcpp/rclcpp.hpp"

namespace
{

constexpr char kTopicName[] = "/race_command";
constexpr char kFrameId[] = "map";
constexpr auto kPublisherSetupDelay = std::chrono::milliseconds(200);
constexpr auto kPublishFlushDelay = std::chrono::milliseconds(200);

void printUsage(const char * program_name)
{
  std::fprintf(
    stderr, "Usage: %s <start|stop|reset>\n", program_name != nullptr ? program_name : "race_command_cli");
}

bool parseCommand(
  const std::string_view argument, std::uint8_t & command_value, const char *& command_name)
{
  if (argument == "start") {
    command_value = race_interfaces::msg::RaceCommand::START;
    command_name = "START";
    return true;
  }

  if (argument == "stop") {
    command_value = race_interfaces::msg::RaceCommand::STOP;
    command_name = "STOP";
    return true;
  }

  if (argument == "reset") {
    command_value = race_interfaces::msg::RaceCommand::RESET;
    command_name = "RESET";
    return true;
  }

  return false;
}

}  // namespace

int main(int argc, char ** argv)
{
  if (argc != 2) {
    printUsage(argc > 0 ? argv[0] : nullptr);
    return 1;
  }

  std::uint8_t command_value = 0U;
  const char * command_name = nullptr;
  if (!parseCommand(argv[1], command_value, command_name)) {
    std::fprintf(stderr, "Invalid command: %s\n", argv[1]);
    printUsage(argv[0]);
    return 1;
  }

  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("race_command_cli");
  auto publisher = node->create_publisher<race_interfaces::msg::RaceCommand>(kTopicName, 10);

  rclcpp::sleep_for(kPublisherSetupDelay);

  race_interfaces::msg::RaceCommand message;
  message.header.stamp = node->get_clock()->now();
  message.header.frame_id = kFrameId;
  message.command = command_value;
  publisher->publish(message);

  RCLCPP_INFO(node->get_logger(), "sent %s", command_name);

  rclcpp::spin_some(node);
  rclcpp::sleep_for(kPublishFlushDelay);
  rclcpp::spin_some(node);

  rclcpp::shutdown();
  return 0;
}
