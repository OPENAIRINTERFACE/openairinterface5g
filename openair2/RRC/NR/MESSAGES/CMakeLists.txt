set(NR_RRC_VERSION 17 3 0)
make_version(NR_RRC_cc ${NR_RRC_VERSION})
string(REPLACE ";" "." NR_RRC_RELEASE "${NR_RRC_VERSION}")

if(NR_RRC_RELEASE VERSION_EQUAL "16.4.1")
  include(ASN.1/nr-rrc-16.4.1.cmake)
elseif(NR_RRC_RELEASE VERSION_EQUAL "17.3.0")
  include(ASN.1/nr-rrc-17.3.0.cmake)
else()
  message(FATAL_ERROR "unknown NR_RRC_RELEASE ${NR_RRC_RELEASE}")
endif()

add_custom_command(OUTPUT ${nr_rrc_source} ${nr_rrc_headers}
  COMMAND ASN1C_PREFIX=NR_ ${ASN1C_EXEC} -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -no-gen-example -findirect-choice -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}
  COMMENT "Generating NR RRC source file from ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}"
)

# Many other targets include NR RRC ASN.1 headers, without actually needing the
# actual library (asn1_nr_rrc). Therefore, below we create the INTERFACE
# library asn1_nr_rrc_hdrs that only contains these headers. To actually wait
# that the headers are created in add_custom_command, we use an intermediary
# add_custom_target which create a target that waits for the headers, and make
# a dependency from asn1_nr_rrc_hdrs to gen_nr_rrc_hdrs, which is the
# recommended way, see https://gitlab.kitware.com/cmake/cmake/-/issues/17366.
# In newer cmake version (e.g., 3.20), it is also possible to have
# ${nr_rrc_headers} directly in the add_library(asn1_nr_rrc_hdrs) statement,
# without the need for asn1_nr_rrc_hdrs
add_custom_target(gen_nr_rrc_hdrs DEPENDS ${nr_rrc_headers})
add_library(asn1_nr_rrc_hdrs INTERFACE)
add_dependencies(asn1_nr_rrc_hdrs gen_nr_rrc_hdrs)
target_include_directories(asn1_nr_rrc_hdrs INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_library(asn1_nr_rrc ${nr_rrc_source})
target_link_libraries(asn1_nr_rrc PUBLIC asn1_nr_rrc_hdrs)
target_compile_options(asn1_nr_rrc
                         PRIVATE -DASN_DISABLE_OER_SUPPORT -w
                         PUBLIC -DNR_RRC_VERSION=${NR_RRC_RELEASE})
