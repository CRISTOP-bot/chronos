find_package(GTest REQUIRED)

add_executable(chronos_tests
  test_events.cpp
  test_storage.cpp
  test_timeline.cpp
  test_state_diff.cpp
  test_ptrace.cpp
  test_checkpoint_restore.cpp
)
target_link_libraries(chronos_tests PRIVATE chronos_core chronos_instrumentation GTest::gtest GTest::gtest_main)
target_include_directories(chronos_tests PRIVATE ${CMAKE_SOURCE_DIR}/instrumentation/include)
include(GoogleTest)
gtest_discover_tests(chronos_tests)
