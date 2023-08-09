set(F1AP_VERSION 16 3 1)
make_version(F1AP_cc ${F1AP_VERSION})
string(REPLACE ";" "." F1AP_RELEASE "${F1AP_VERSION}")

if(F1AP_RELEASE VERSION_EQUAL "16.3.1")
  include(ASN1/f1ap-16.3.1.cmake)
else()
  message(FATAL_ERROR "unknown F1AP_RELEASE ${F1AP_RELEASE}")
endif()

add_custom_command(OUTPUT ${f1ap_source} ${f1ap_headers}
  COMMAND ASN1C_PREFIX=F1AP_ ${ASN1C_EXEC} -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${F1AP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${F1AP_GRAMMAR}
  COMMENT "Generating F1AP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${F1AP_GRAMMAR}"
)

add_library(asn1_f1ap ${f1ap_source})
target_include_directories(asn1_f1ap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_f1ap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
