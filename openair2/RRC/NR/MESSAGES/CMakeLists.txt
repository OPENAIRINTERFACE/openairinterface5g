set(NR_RRC_VERSION 16 4 1)
make_version(NR_RRC_cc ${NR_RRC_VERSION})
string(REPLACE ";" "." NR_RRC_RELEASE "${NR_RRC_VERSION}")

if(NR_RRC_RELEASE VERSION_EQUAL "16.4.1")
  include(ASN.1/nr-rrc-16.4.1.cmake)
else()
  message(FATAL_ERROR "unknown NR_RRC_RELEASE ${NR_RRC_RELEASE}")
endif()

add_custom_command(OUTPUT ${nr_rrc_source} ${nr_rrc_headers}
  COMMAND ASN1C_PREFIX=NR_ asn1c -pdu=all -fcompound-names -gen-PER -no-gen-OER -no-gen-example -findirect-choice -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}
  COMMENT "Generating NR RRC source file from ${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}"
)

add_library(asn1_nr_rrc ${nr_rrc_source})
target_include_directories(asn1_nr_rrc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_nr_rrc
                         PRIVATE -DASN_DISABLE_OER_SUPPORT -w
                         PUBLIC -DNR_RRC_VERSION=${NR_RRC_RELEASE})
