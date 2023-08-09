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
#! \file common/utils/websrv/CMakelists.txt
#* \brief: build rules and checks for web server shared libraries
#* \author Francois TABURET
#* \date 2023
#* \version 1.0
#* \company NOKIA BellLabs France
#* \email: francois.taburet@nokia-bell-labs.com
#* \note
#* \warning
#*/
find_library(ULFIUS NAMES "libulfius.so" REQUIRED)
if (NOT ULFIUS)
  message(FATAL_ERROR "ulfius library (https://github.com/babelouest/ulfius) not found, install libulfius-dev (ubuntu) if you need to build websrv back-end")
else()
  message(STATUS "found libulfius for websrv")
endif()

find_library(JSON NAMES "libjansson.so" REQUIRED)
if (NOT JSON)
  message(FATAL_ERROR "libjansson not found, install libjansson-dev for ubuntu, jansson-devel for fedora if you need to build websrv back-end")
else()
  message(STATUS "found libjansson for websrv")
endif()

find_program(NPM NAMES npm REQUIRED)
if (NOT NPM)
  message(FATAL_ERROR "npm is not installed, frontend can't be built. Possibly install npm, package is available for ubuntu and fedora")
else()
  message(STATUS "found npm for websrv")
endif()


# build the backend ( the embedded web server)

message(STATUS "Configuring webserver backend")
add_library(websrv MODULE
            websrv.c
            websrv_websockets.c
            websrv_scope.c
            websrv_noforms.c
            websrv_scope.c
            websrv_utils.c
            ../../../openair1/PHY/TOOLS/nr_phy_scope.c
)
target_link_libraries(websrv PRIVATE ulfius jansson)
target_compile_definitions(websrv PUBLIC WEBSRVSCOPE)
# put websrv into build root and not under build/common/utils/websrv/
set_target_properties(websrv PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../..)

# build the frontend ( loaded from web server by browsers)

add_custom_target(
  websrvfront_installjsdep
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/frontend
  COMMAND npm install
  DEPENDS frontend/package-lock.json
)

message(STATUS "Configuring webserver frontend")
add_custom_target(
  websrvfront
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/frontend
  COMMAND npm run build
  #COMMAND npm run builddev
  DEPENDS websrvfront_installjsdep
)
add_dependencies(websrv websrvfront)
# copy frontend files to a websrv subdirectory of where oai copy all built files
add_custom_command(TARGET websrvfront
                   POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/frontend/dist/softmodem-ngx ${OPENAIR_BIN_DIR}/../websrv
                              COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/helpfiles ${OPENAIR_BIN_DIR}/../websrv/helpfiles
                   COMMENT "Moving frontend files to:\n   ${OPENAIR_BIN_DIR}/../websrv \n " )
