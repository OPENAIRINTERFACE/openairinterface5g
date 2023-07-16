set(XNAP_VERSION 16 2 0)
make_version(XNAP_cc ${XNAP_VERSION})
#add_definitions(-DXNAP_VERSION=${XNAP_cc})
string(REPLACE ";" "." XNAP_RELEASE "${XNAP_VERSION}")

if(XNAP_RELEASE VERSION_EQUAL "16.2.0")
  include(ASN1/xnap_R16.2.0.cmake)
else()
  message(FATAL_ERROR "unknown XNAP_RELEASE ${XNAP_RELEASE}")
endif()

add_custom_command(OUTPUT ${xnap_source} ${xnap_headers}
  COMMAND ASN1C_PREFIX=XNAP_ ${ASN1C_EXEC} -pdu=all -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -findirect-choice -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${XNAP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${XNAP_GRAMMAR}
  COMMENT "Generating XNAP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${XNAP_GRAMMAR}"
	)

add_library(asn1_xnap ${xnap_source})
target_include_directories(asn1_xnap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_xnap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)

