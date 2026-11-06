# Core library
add_library(chronos_core STATIC
  common/src/types.cpp
  common/src/error.cpp
  events/src/event.cpp
  events/src/event_log.cpp
  storage/src/format.cpp
  storage/src/writer.cpp
  storage/src/reader.cpp
  recording/src/session.cpp
  recording/src/recording_engine.cpp
  execution/src/session_manager.cpp
  memory/src/region.cpp
  memory/src/checkpoint_restore.cpp
  memory/src/proc_maps.cpp
  timeline/src/timeline.cpp
  analysis/src/state_diff.cpp
)

target_include_directories(chronos_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chronos_core PUBLIC
  z
  zstd
)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(chronos_core PUBLIC Threads::Threads)

set_target_properties(chronos_core PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
