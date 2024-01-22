# Check that the submodule exists or init+update if not 
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/flexric/CMakeLists.txt)
  message(STATUS "E2AP submoduled not detected, therefore $git submodule init && git submodule update.")
  execute_process(COMMAND git submodule init)
  execute_process(COMMAND git submodule update) 
else()
  message(STATUS "E2AP submodule detected.")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/flexric/CMakeLists.txt)
  message(FATAL_ERROR "The submodules for E2 agent were not downloaded!")
endif()


set(BUILDING_LIBRARY "STATIC" CACHE STRING "Static or dynamic library")
set_property(CACHE BUILDING_LIBRARY PROPERTY STRINGS "STATIC" "DYNAMIC")
message(STATUS "Selected LIBRARY TYPE: ${BUILDING_LIBRARY}")


set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(BUILDING_LIBRARY STREQUAL "STATIC")
  add_compile_options("-W;-Wall;-Wextra;-g;-Wno-unused-result;")
elseif(BUILDING_LIBRARY STREQUAL "DYNAMIC")
  # -fPIC flag
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_compile_options("-W;-Wall;-Wextra;-g")
else()
  message(FATAL_ERROR "Unknown building type. Either choose a static or a dynamic library")
endif()

#######
## E2AP encoding and versioning 
#######

set(E2AP_ENCODING "ASN" CACHE STRING "The E2AP encoding to use")
set_property(CACHE E2AP_ENCODING PROPERTY STRINGS "ASN" "FLATBUFFERS")
message(STATUS "Selected E2AP_ENCODING: ${E2AP_ENCODING}")


#######
## Service Models 
#######

add_definitions(-DSERVICE_MODEL_DIR_PATH="${SM_DIR_PATH}/")

# KPM service Model encoding definitions
set(SM_ENCODING_KPM "ASN" CACHE STRING "The KPM SM encoding to use")
set_property(CACHE SM_ENCODING_KPM PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected KPM SM_ENCODING: ${SM_ENCODING_KPM}")

# RC service Model encoding definitions
set(SM_ENCODING_RC "ASN" CACHE STRING "The RC SM encoding to use")
set_property(CACHE SM_ENCODING_RC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected RC SM_ENCODING: ${SM_ENCODING_RC}")

# MAC Service Model
set(SM_ENCODING_MAC "PLAIN" CACHE STRING "The MAC SM encoding to use")
set_property(CACHE SM_ENCODING_MAC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected MAC SM_ENCODING: ${SM_ENCODING_MAC}")

# RLC Service Model
set(SM_ENCODING_RLC "PLAIN" CACHE STRING "The RLC SM encoding to use")
set_property(CACHE SM_ENCODING_RLC PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected RLC SM_ENCODING: ${SM_ENCODING_RLC}")

# PDCP Service Model
set(SM_ENCODING_PDCP "PLAIN" CACHE STRING "The PDCP SM encoding to use")
set_property(CACHE SM_ENCODING_PDCP PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected PDCP SM_ENCODING: ${SM_ENCODING_PDCP}")

# SLICE Service Model
set(SM_ENCODING_SLICE "PLAIN" CACHE STRING "The SLICE SM encoding to use")
set_property(CACHE SM_ENCODING_SLICE PROPERTY STRINGS "PLAIN" "ASN" "FLATBUFFERS")
message(STATUS "Selected SLICE SM_ENCODING: ${SM_ENCODING_SLICE}")

# GTP Service Model
set(SM_ENCODING_GTP "PLAIN" CACHE STRING "The GTP SM encoding to use")
set_property(CACHE SM_ENCODING_GTP PROPERTY STRINGS "PLAIN")
message(STATUS "Selected GTP SM_ENCODING: ${SM_ENCODING_GTP}")


if(E2AP_VERSION STREQUAL "E2AP_V1")
  add_library(e2ap_ran_func_obj OBJECT 
              flexric/src/lib/e2ap/v1_01/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT 
              flexric/src/lib/e2ap/v1_01/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT 
              flexric/src/lib/e2ap/v1_01/e2ap_types/common/e2ap_global_node_id.c )
elseif(E2AP_VERSION STREQUAL "E2AP_V2")
  add_library(e2ap_ran_func_obj OBJECT 
              flexric/src/lib/e2ap/v2_03/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT 
              flexric/src/lib/e2ap/v2_03/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT 
              flexric/src/lib/e2ap/v2_03/e2ap_types/common/e2ap_global_node_id.c )
elseif(E2AP_VERSION STREQUAL "E2AP_V3")
  add_library(e2ap_ran_func_obj OBJECT 
              flexric/src/lib/e2ap/v3_01/e2ap_types/common/e2ap_ran_function.c )
  add_library(e2ap_plmn_obj OBJECT 
              flexric/src/lib/e2ap/v3_01/e2ap_types/common/e2ap_plmn.c )
  add_library(e2ap_global_node_id_obj OBJECT 
              flexric/src/lib/e2ap/v3_01/e2ap_types/common/e2ap_global_node_id.c )
else()
  message(FATAL_ERROR "E2AP Unknown version selected")
endif()




########
### Flatbuffer 
########
set(FlatCC_INCLUDE_DIR "" CACHE STRING "The Flatbuffers include directory")
set(FlatCC_LIB_DIR "" CACHE STRING "The Flatbuffers lib directory")

if(E2AP_ENCODING STREQUAL "FLATBUFFERS")
  find_library(FlatCC
    NAMES flatccrt_d
    HINTS ${FlatCC_LIB_DIR} 
    )
endif()

include_directories(flexric/src)
add_subdirectory(flexric/src/agent)
add_subdirectory(flexric/src/lib)
add_subdirectory(flexric/src/sm)
add_subdirectory(flexric/src/util)
add_subdirectory(RAN_FUNCTION)

add_library(e2_agent_arg STATIC e2_agent_arg.c)
add_dependencies(e2_agent_arg generate_T)
