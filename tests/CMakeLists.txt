cmake_minimum_required(VERSION 3.14...3.27)

# Prefer a system-installed Catch2; fall back to FetchContent for CI environments
# that install it via package manager (ubuntu: apt install catch2).
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(Catch2)
endif()

# ── Unit tests (no plugin loading) ──────────────────────────────────────────
add_executable(pugg_unit_tests unit_tests.cpp)
target_link_libraries(pugg_unit_tests PRIVATE pugg Catch2::Catch2WithMain)

# ── Integration tests (requires plugin shared libraries) ────────────────────
add_executable(pugg_integration_tests integration_tests.cpp)
target_link_libraries(pugg_integration_tests
  PRIVATE pugg animal Catch2::Catch2WithMain ${CMAKE_DL_LIBS})

# Tell the integration test where to find the plugin .so/.dll files at runtime.
target_compile_definitions(pugg_integration_tests PRIVATE
  PANGEA_PLUGIN_PATH="$<TARGET_FILE:PangeaAnimals>"
  PANTHALASSA_PLUGIN_PATH="$<TARGET_FILE:PanthalassaAnimals>"
)

add_dependencies(pugg_integration_tests PangeaAnimals PanthalassaAnimals)

include(CTest)
include(Catch)
catch_discover_tests(pugg_unit_tests)
catch_discover_tests(pugg_integration_tests)
