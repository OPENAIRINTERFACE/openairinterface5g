set(NGAP_VERSION 15 8 0)
make_version(NGAP_cc ${NGAP_VERSION})
#add_definitions(-DNGAP_VERSION=${NGAP_cc})
string(REPLACE ";" "." NGAP_RELEASE "${NGAP_VERSION}")

if(NGAP_RELEASE VERSION_EQUAL "15.8.0")
  include(ASN1/ngap-15.8.0.cmake)
else()
  message(FATAL_ERROR "unknown NGAP_RELEASE ${NGAP_RELEASE}")
endif()

add_custom_command(OUTPUT ${ngap_source} ${ngap_headers}
  COMMAND ASN1C_PREFIX=NGAP_ ${ASN1C_EXEC} -pdu=all -fcompound-names -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -no-gen-example -fno-include-deps -findirect-choice -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${NGAP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NGAP_GRAMMAR}
  COMMENT "Generating NGAP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${NGAP_GRAMMAR}"
)

add_library(asn1_ngap ${ngap_source})
target_include_directories(asn1_ngap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_ngap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
