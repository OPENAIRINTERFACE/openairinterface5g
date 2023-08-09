add_library(tracer_utils OBJECT configuration.c database.c event.c handler.c utils.c)
target_link_libraries(tracer_utils PUBLIC m pthread)

add_library(tracer_events OBJECT event_selector.c)

add_executable(record record.c)
target_link_libraries(record PRIVATE tracer_utils)

add_executable(replay replay.c)
target_link_libraries(replay PRIVATE tracer_utils)

add_executable(extract_config extract_config.c)
target_link_libraries(extract_config PRIVATE tracer_utils)

add_executable(extract_input_subframe extract_input_subframe.c)
target_link_libraries(extract_input_subframe PRIVATE tracer_utils)

add_executable(extract_output_subframe extract_output_subframe.c)
target_link_libraries(extract_output_subframe PRIVATE tracer_utils)

add_executable(extract extract.c)
target_link_libraries(extract PRIVATE tracer_utils)

add_executable(macpdu2wireshark macpdu2wireshark.c)
target_link_libraries(macpdu2wireshark PRIVATE tracer_utils)

add_executable(multi multi.c)
target_link_libraries(multi PRIVATE tracer_utils T)
target_include_directories(multi PRIVATE ..)

add_custom_target(T_tools)
add_dependencies(T_tools
                 record replay extract_config extract_input_subframe
                 extract_output_subframe extract macpdu2wireshark multi)
add_dependencies(nr-softmodem T_tools)
add_dependencies(nr-uesoftmodem T_tools)
add_dependencies(lte-softmodem T_tools)
add_dependencies(lte-uesoftmodem T_tools)

# Some T tracer tools need additional libraries (Xft, also PNG) that come with
# a long list of dependencies. We put it behind a separate option for
# GUI-related tools so they can be added on demand, if necessary.
add_boolean_option(T_TRACER_GUI OFF  "Compile T tracer GUI tools" OFF)
if(T_TRACER_GUI)
  add_executable(textlog textlog.c)
  target_link_libraries(textlog PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)

  find_library(png png REQUIRED)
  add_executable(enb enb.c)
  target_link_libraries(enb PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)
  target_link_libraries(enb PRIVATE png)

  add_executable(gnb gnb.c)
  target_link_libraries(gnb PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)
  target_link_libraries(gnb PRIVATE png)

  add_executable(ue ue.c)
  target_link_libraries(ue PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)
  target_link_libraries(ue PRIVATE png)

  add_executable(vcd vcd.c)
  target_link_libraries(vcd PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)

  add_executable(to_vcd to_vcd.c)
  target_link_libraries(to_vcd PRIVATE tracer_utils tracer_filter tracer_gui
                        tracer_logger tracer_view tracer_events)

  add_subdirectory(gui)
  add_subdirectory(filter)
  add_subdirectory(logger)
  add_subdirectory(view)

  add_dependencies(T_tools textlog enb gnb ue vcd to_vcd)
endif()
