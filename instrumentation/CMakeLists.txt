add_library(chronos_instrumentation STATIC
  src/ptrace_tracer.cpp
)
target_include_directories(chronos_instrumentation PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/core/include
)
target_link_libraries(chronos_instrumentation PUBLIC chronos_core)
