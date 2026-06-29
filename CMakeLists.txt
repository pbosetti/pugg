cmake_minimum_required(VERSION 3.14...3.27)

project(
  puggSuite
  VERSION 1.0.4
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/pugg)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Build examples and tests only when built as the top-level project
  add_subdirectory(src/test_projects/Animal)
  add_subdirectory(src/test_projects/Zoo)
  add_subdirectory(src/test_projects/PanthalassaAnimals)
  add_subdirectory(src/test_projects/PangeaAnimals)

  enable_testing()
  add_subdirectory(tests)
endif()
