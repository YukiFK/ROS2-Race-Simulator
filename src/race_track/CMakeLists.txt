cmake_minimum_required(VERSION 3.8)
project(race_track)

find_package(ament_cmake REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(${PROJECT_NAME}
  src/geometry.cpp
  src/track_loader.cpp
  src/track_validator.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp)

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

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS ${PROJECT_NAME} track_demo race_progress_demo
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
endif()

ament_export_include_directories(include)
ament_export_dependencies(yaml-cpp)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)

ament_package()
