find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(chronos_gui
  src/main.cpp
  src/mainwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/chronos/gui/mainwindow.h
)
set_target_properties(chronos_gui PROPERTIES AUTOMOC ON)
target_include_directories(chronos_gui PRIVATE
  ${CMAKE_SOURCE_DIR}/core/include
  ${CMAKE_SOURCE_DIR}/gui/include
)
target_link_libraries(chronos_gui PRIVATE chronos_core chronos_instrumentation Qt6::Widgets)
target_include_directories(chronos_gui PRIVATE ${CMAKE_SOURCE_DIR}/instrumentation/include)
set_target_properties(chronos_gui PROPERTIES AUTOMOC ON)
