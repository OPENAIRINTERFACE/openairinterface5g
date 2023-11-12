if (CROSS_COMPILE)
  find_package(genids)
else()
  add_executable(genids genids.c)
  set_target_properties(genids PROPERTIES RUNTIME_OUTPUT_DIRECTORY .)
  export(TARGETS genids FILE
          "${CMAKE_BINARY_DIR}/genidsConfig.cmake")
endif()
add_custom_command(OUTPUT T_IDs.h
                   COMMAND genids ${CMAKE_CURRENT_SOURCE_DIR}/T_messages.txt T_IDs.h
                   DEPENDS genids T_messages.txt
                   COMMENT "Generating T_IDs.h from T_messages.txt"
)
add_custom_command(OUTPUT T_messages.txt.h
                   COMMAND xxd -i T_messages.txt ${CMAKE_CURRENT_BINARY_DIR}/T_messages.txt.h
                   DEPENDS T_messages.txt
                   COMMENT "Generating T_messages.txt.h from T_messages.txt"
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (CROSS_COMPILE)
  find_package(_check_vcd)
else ()
  add_executable(_check_vcd check_vcd.c tracer/database.c tracer/utils.c T_IDs.h T_messages.txt.h)
  target_include_directories(_check_vcd PRIVATE
                             .. ../itti ../../../openair2/COMMON tracer
                             ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_options(_check_vcd PRIVATE -Wall)
  target_link_libraries(_check_vcd PRIVATE m pthread)
  set_target_properties(_check_vcd PROPERTIES RUNTIME_OUTPUT_DIRECTORY .)
  export(TARGETS _check_vcd FILE
            "${CMAKE_BINARY_DIR}/_check_vcdConfig.cmake")
endif()
add_custom_target(check_vcd
                  COMMAND _check_vcd ${CMAKE_CURRENT_SOURCE_DIR}/T_messages.txt ${CMAKE_CURRENT_SOURCE_DIR}/../LOG/vcd_signal_dumper.h
                  DEPENDS _check_vcd
                  COMMENT "Checking validity of VCD files"
)

# This rule is specifically needed to generate T files before anything else in
# a project that uses the T.
# See root CMakeLists.txt, there are some 'add_dependencies' showing that.
# Basically we create a custom target and we make other targets depend on it.
# That forces cmake to generate T files before anything else.
add_custom_target(generate_T DEPENDS T_IDs.h check_vcd)

# even if T is off, we need the headers as many targets depend on it
# for this, create an INTERFACE (header-only) library T_headers. To ensure the
# headers have really been created, we make this headers library explicitly
# depend on the generated headers.
add_library(T_headers INTERFACE)
add_dependencies(T_headers T_IDs.h T_messages.txt.h)
target_include_directories(T_headers INTERFACE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(T_TRACER)
  add_library(T STATIC T.c local_tracer.c T_IDs.h T_messages.txt.h)
  target_link_libraries(T PUBLIC rt)
  target_link_libraries(T PRIVATE CONFIG_LIB T_headers)

  add_subdirectory(tracer)
endif()
