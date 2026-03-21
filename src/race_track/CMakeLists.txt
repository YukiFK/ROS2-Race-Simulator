cmake_minimum_required(VERSION 3.8)
project(race_track)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(race_interfaces REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(${PROJECT_NAME}
  src/completion_evaluator.cpp
  src/geometry.cpp
  src/lap_event_assembler.cpp
  src/progress_tracker.cpp
  src/race_state_assembler.cpp
  src/single_vehicle_runtime.cpp
  src/track_loader.cpp
  src/track_validator.cpp
  src/vehicle_race_status_assembler.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp)
ament_target_dependencies(${PROJECT_NAME} race_interfaces)

add_executable(track_demo
  src/track_demo.cpp
)
target_compile_definitions(track_demo PRIVATE
  RACE_TRACK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(track_demo PRIVATE ${PROJECT_NAME})

add_executable(race_progress_demo
  src/race_progress_demo.cpp
)
target_compile_definitions(race_progress_demo PRIVATE
  RACE_TRACK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(race_progress_demo PRIVATE ${PROJECT_NAME})

add_executable(race_message_demo
  src/race_message_demo.cpp
)
target_compile_definitions(race_message_demo PRIVATE
  RACE_TRACK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(race_message_demo
  ${PROJECT_NAME}
)
ament_target_dependencies(race_message_demo race_interfaces)

add_executable(race_progress_publisher
  src/race_progress_publisher.cpp
)
target_compile_definitions(race_progress_publisher PRIVATE
  RACE_TRACK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(race_progress_publisher
  ${PROJECT_NAME}
)
ament_target_dependencies(race_progress_publisher rclcpp race_interfaces)

add_executable(race_progress_monitor
  src/race_progress_monitor.cpp
)
ament_target_dependencies(race_progress_monitor rclcpp race_interfaces)

add_executable(race_command_cli
  src/race_command_cli.cpp
)
ament_target_dependencies(race_command_cli rclcpp race_interfaces)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS ${PROJECT_NAME} track_demo race_progress_demo race_message_demo race_progress_publisher
    race_progress_monitor race_command_cli
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_track_loader
    test/test_track_loader.cpp
  )
  target_link_libraries(test_track_loader ${PROJECT_NAME})

  ament_add_gtest(test_track_validator
    test/test_track_validator.cpp
  )
  target_link_libraries(test_track_validator ${PROJECT_NAME})

  ament_add_gtest(test_geometry
    test/test_geometry.cpp
  )
  target_link_libraries(test_geometry ${PROJECT_NAME})

  ament_add_gtest(test_progress_tracker
    test/test_progress_tracker.cpp
  )
  target_link_libraries(test_progress_tracker ${PROJECT_NAME})

  ament_add_gtest(test_completion_evaluator
    test/test_completion_evaluator.cpp
  )
  target_link_libraries(test_completion_evaluator ${PROJECT_NAME})

  ament_add_gtest(test_race_state_assembler
    test/test_race_state_assembler.cpp
  )
  target_link_libraries(test_race_state_assembler ${PROJECT_NAME})

  ament_add_gtest(test_lap_event_assembler
    test/test_lap_event_assembler.cpp
  )
  target_link_libraries(test_lap_event_assembler ${PROJECT_NAME})

  ament_add_gtest(test_vehicle_race_status_assembler
    test/test_vehicle_race_status_assembler.cpp
  )
  target_link_libraries(test_vehicle_race_status_assembler ${PROJECT_NAME})

  ament_add_gtest(test_single_vehicle_runtime
    test/test_single_vehicle_runtime.cpp
  )
  target_link_libraries(test_single_vehicle_runtime ${PROJECT_NAME})
endif()

ament_export_include_directories(include)
ament_export_dependencies(race_interfaces)
ament_export_dependencies(yaml-cpp)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)

ament_package()
