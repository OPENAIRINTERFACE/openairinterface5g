#set(NRPPA_VERSION 16 78 0)
#make_version(NRPPA_cc ${NRPPA_VERSION})
#add_definitions(-DNRPPA_VERSION=${NRPPA_cc})
#string(REPLACE ";" "." NRPPA_RELEASE "${NRPPA_VERSION}")

#if(NRPPA_RELEASE VERSION_EQUAL "16.78.0")
include(ASN1/38455.cmake)
#else()
#  message(FATAL_ERROR "unknown NRPPA_RELEASE ${NRPPA_RELEASE}")
#endif()

add_custom_command(OUTPUT ${nrppa_source} ${nrppa_headers}
  COMMAND ASN1C_PREFIX=NRPPA_ ${ASN1C_EXEC} -gen-APER -gen-UPER -no-gen-OER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${NRPPA_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NRPPA_GRAMMAR}
  COMMENT "Generating NRPPA source files from ${CMAKE_CURRENT_SOURCE_DIR}/${NRPPA_GRAMMAR}"
)

add_library(asn1_nrppa ${nrppa_source})
target_include_directories(asn1_nrppa PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_nrppa PRIVATE -DASN_DISABLE_OER_SUPPORT -w)


