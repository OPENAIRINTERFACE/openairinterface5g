set(E1AP_VERSION 16 78 0)
make_version(E1AP_cc ${E1AP_VERSION})
#add_definitions(-DE1AP_VERSION=${E1AP_cc})
string(REPLACE ";" "." E1AP_RELEASE "${E1AP_VERSION}")

if(E1AP_RELEASE VERSION_EQUAL "16.78.0")
  include(ASN.1/38463-g80.R16.78.0.cmake)
else()
  message(FATAL_ERROR "unknown E1AP_RELEASE ${E1AP_RELEASE}")
endif()

add_custom_command(OUTPUT ${e1ap_source} ${e1ap_headers}
  COMMAND ASN1C_PREFIX=E1AP_ ${ASN1C_EXEC} -gen-APER -gen-UPER -no-gen-JER -no-gen-BER -no-gen-OER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${E1AP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${E1AP_GRAMMAR}
  COMMENT "Generating E1AP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${E1AP_GRAMMAR}"
)

add_library(asn1_e1ap ${e1ap_source})
target_include_directories(asn1_e1ap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_e1ap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
