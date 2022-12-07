set(LTE_RRC_VERSION 15 6 0)
make_version(LTE_RRC_cc ${LTE_RRC_VERSION})
string(REPLACE ";" "." LTE_RRC_RELEASE "${LTE_RRC_VERSION}")

if(LTE_RRC_RELEASE VERSION_EQUAL "15.6.0")
  include(ASN.1/lte-rrc-15.6.0.cmake)
else()
  message(FATAL_ERROR "unknown LTE_RRC_RELEASE ${LTE_RRC_RELEASE}")
endif()

add_custom_command(OUTPUT ${lte_rrc_source} ${lte_rrc_headers}
  COMMAND ASN1C_PREFIX=LTE_ asn1c -pdu=all -fcompound-names -gen-PER -no-gen-OER -no-gen-example -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}
  COMMENT "Generating LTE RRC source file from ${CMAKE_CURRENT_SOURCE_DIR}/${LTE_RRC_GRAMMAR}"
)

add_library(asn1_lte_rrc ${lte_rrc_source})
target_include_directories(asn1_lte_rrc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_lte_rrc
                         PRIVATE -DASN_DISABLE_OER_SUPPORT -w
                         PUBLIC -DLTE_RRC_VERSION=${LTE_RRC_RELEASE})
