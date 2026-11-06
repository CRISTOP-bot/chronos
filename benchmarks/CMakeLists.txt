add_executable(chronos_bench bench_storage.cpp)
target_link_libraries(chronos_bench PRIVATE chronos_core)
target_include_directories(chronos_bench PRIVATE ${CMAKE_SOURCE_DIR}/core/include)
