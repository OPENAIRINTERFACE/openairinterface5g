set(LTE_RRC_VERSION 16 13 0)
make_version(LTE_RRC_cc ${LTE_RRC_VERSION})
string(REPLACE ";" "." LTE_RRC_RELEASE "${LTE_RRC_VERSION}")

if(LTE_RRC_RELEASE VERSION_EQUAL "16.13.0")
  include(ASN.1/lte-rrc-16.13.0.cmake)
elseif(LTE_RRC_RELEASE VERSION_EQUAL "15.6.0")
  include(ASN.1/lte-rrc-15.6.0.cmake)
else()
  message(FATAL_ERROR "unknown LTE_RRC_RELEASE ${LTE_RRC_RELEASE}")
endif()

add_custom_command(OUTPUT ${lte_rrc_source} ${lte_rrc_headers}
  COMMAND ASN1C_PREFIX=LTE_ ${ASN1C_EXEC} -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -no-gen-example -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}
  COMMENT "Generating LTE RRC source file from ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}"
)

# Many other targets include LTE RRC ASN.1 headers, without actually needing
# the actual library (asn1_lte_rrc). Therefore, below we create the INTERFACE
# library asn1_lte_rrc_hdrs that only contains these headers. To actually wait
# that the headers are created in add_custom_command, we use an intermediary
# add_custom_target which create a target that waits for the headers, and make
# a dependency from asn1_lte_rrc_hdrs to gen_lte_rrc_hdrs, which is the
# recommended way, see https://gitlab.kitware.com/cmake/cmake/-/issues/17366.
# In newer cmake version (e.g., 3.20), it is also possible to have
# ${lte_rrc_headers} directly in the add_library(asn1_lte_rrc_hdrs) statement,
# without the need for asn1_lte_rrc_hdrs
add_custom_target(gen_lte_rrc_hdrs DEPENDS ${lte_rrc_headers})
add_library(asn1_lte_rrc_hdrs INTERFACE)
add_dependencies(asn1_lte_rrc_hdrs gen_lte_rrc_hdrs)
target_include_directories(asn1_lte_rrc_hdrs INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_library(asn1_lte_rrc ${lte_rrc_source})
target_link_libraries(asn1_lte_rrc PUBLIC asn1_lte_rrc_hdrs)
target_compile_options(asn1_lte_rrc
                         PRIVATE -DASN_DISABLE_OER_SUPPORT -w
                         PUBLIC -DLTE_RRC_VERSION=${LTE_RRC_RELEASE})
