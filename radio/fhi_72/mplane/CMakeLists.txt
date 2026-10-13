# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_library(oran_fhlib_5g_mplane MODULE
            ../oran_isolate.c
            ../oaioran.c
            ../oran-config.c
            ../oran-init.c
)
add_dependencies(oran_fhlib_5g_mplane oran_fhlib_5g)

if(xran_FOUND)
  if(xran_VERSION VERSION_GREATER_EQUAL 5 AND xran_VERSION VERSION_LESS 6)
    message(FATAL_ERROR "xran E release not supported starting from tag 2026.w08.\nPlease switch to K version ${K_VERSION}")
  elseif(xran_VERSION VERSION_GREATER_EQUAL 6 AND xran_VERSION VERSION_LESS 7)
    message(FATAL_ERROR "xran F release not supported starting from tag 2026.w29.\nPlease switch to K version ${K_VERSION}")
  elseif(xran_VERSION VERSION_EQUAL K_VERSION)
    target_compile_definitions(oran_fhlib_5g_mplane PRIVATE K_RELEASE)
  endif()
elseif(NOT xran_FOUND AND xran_DOWNLOAD)
  target_compile_definitions(oran_fhlib_5g_mplane PRIVATE K_RELEASE)
endif()

# Ignore xran-specific warning: we don't care/can't change the following warning, so suppress
# alignment 1 of 'struct XYZ' is less than 2
add_compile_options(-Wno-packed-not-aligned)

set_target_properties(oran_fhlib_5g_mplane PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -march=native")
target_link_libraries(oran_fhlib_5g_mplane PRIVATE xran::xran PkgConfig::dpdk_fh log_headers pthread dl rt m numa)

target_compile_definitions(oran_fhlib_5g_mplane PRIVATE OAI_MPLANE)

###################################################

target_sources(oran_fhlib_5g_mplane PRIVATE
               init-mplane.c
               connect-mplane.c
               rpc-send-recv.c
               get-mplane.c
               subscribe-mplane.c
               config-mplane.c
               ru-mplane-api.c
               xml/get-xml.c
               yang/get-yang.c
               yang/create-yang-config.c
)

pkg_check_modules(libyang REQUIRED libyang)
pkg_check_modules(libnetconf2 REQUIRED libnetconf2)

if(libyang_VERSION VERSION_LESS_EQUAL 1.0.240 AND libnetconf2_VERSION VERSION_LESS_EQUAL 1.1.46)
  target_compile_definitions(oran_fhlib_5g_mplane PRIVATE MPLANE_V1)
elseif((libyang_VERSION VERSION_GREATER 1.0.240 AND libyang_VERSION VERSION_LESS 2.1.4))
  message(FATAL_ERROR "Minimum libyang v2 subversion required is v2.1.4, but found ${libyang_VERSION}")
elseif((libnetconf2_VERSION VERSION_GREATER 1.1.46 AND libnetconf2_VERSION VERSION_LESS 2.1.25))
  message(FATAL_ERROR "Minimum libnetconf2 v2 subversion required is v2.1.25, but found ${libnetconf2_VERSION}")
elseif((libyang_VERSION VERSION_GREATER_EQUAL 2.1.4 AND libyang_VERSION VERSION_LESS_EQUAL 2.1.111) AND (libnetconf2_VERSION VERSION_GREATER_EQUAL 2.1.25 AND libnetconf2_VERSION VERSION_LESS_EQUAL 2.1.37))
  target_compile_definitions(oran_fhlib_5g_mplane PRIVATE MPLANE_V2)
else()
  message(FATAL_ERROR "Unknown libyang version ${libyang_VERSION} and libnetconf2 version ${libnetconf2_VERSION}")
endif()

target_include_directories(oran_fhlib_5g_mplane PRIVATE ${libyang_INCLUDE_DIRS} ${libnetconf2_INCLUDE_DIRS})
target_link_libraries(oran_fhlib_5g_mplane PRIVATE ${libyang_LDFLAGS} ${libyang_LINK_LIBRARIES} ${libnetconf2_LINK_LIBRARIES})

pkg_check_modules(libssh REQUIRED libssh)
target_include_directories(oran_fhlib_5g_mplane PRIVATE ${libssh_INCLUDE_DIRS})
target_link_libraries(oran_fhlib_5g_mplane PRIVATE ${libssh_LINK_LIBRARIES})

find_package(LibXml2 REQUIRED)
target_link_libraries(oran_fhlib_5g_mplane PRIVATE LibXml2::LibXml2)

set(YANG_MODELS "${CMAKE_SOURCE_DIR}/radio/fhi_72/mplane/yang/models")
target_compile_definitions(oran_fhlib_5g_mplane PRIVATE YANG_MODELS="${YANG_MODELS}")

message(STATUS "Building FHI72 CUSM library")

set_target_properties(oran_fhlib_5g_mplane PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET oran_fhlib_5g_mplane POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink liboran_fhlib_5g_mplane.so liboai_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
