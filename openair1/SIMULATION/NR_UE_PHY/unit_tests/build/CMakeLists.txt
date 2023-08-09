#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -O0 -g -Wextra -march=native"
)

add_definitions("-DUPGRADE_RAT_NR")
add_definitions("-DDEBUG_PSS_NR")
add_definitions("-DDEBUG_SSS_NR")
add_definitions("-DNR_UNIT_TEST")

set(PACKAGE_NAME "Testbench for unitary tests")
set(PHYSIM True)
set(RF_BOARD None)
set(XFORMS False)
set(ENABLE_ITTI False)
set(DEBUG_PHY False)
set(DEBUG_PSS True)
set(DEBUG_PHY True)
set(DEBUG_PHY_PROC False)
set(DEBUG_DLSCH True)
set(T_TRACER False)

# standard cmake file for nr ue build
include(../../../../../CMakeLists.txt)


set(SRC_UNIT_TESTS 
    ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pss_util_test.c
    ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/dummy_functions.c
    )
    
set(INC_UNIT_TESTS
     ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pss_util_test.h
    )

add_executable(pss_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pss_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(pss_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )
                      
add_executable(sss_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/sss_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(sss_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )

add_executable(frame_config_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/frame_config_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(frame_config_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )
                      
add_executable(harq_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/harq_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(harq_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )
                      
add_executable(srs_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/srs_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(srs_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )
                      
add_executable(pbch_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pbch_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(pbch_test 
                      -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )

add_executable(pucch_uci_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pucch_uci_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(pucch_uci_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )
add_executable(pucch_uci_generator_test ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pucch_uci_generator_test.c ${SRC_UNIT_TESTS} )
target_link_libraries(pucch_uci_generator_test 
                       -Wl,--start-group UTIL SCHED_NR_UE_LIB PHY PHY_COMMON PHY_UE PHY_NR_UE -Wl,--end-group
                      pthread m ${ATLAS_LIBRARIES}
                      )