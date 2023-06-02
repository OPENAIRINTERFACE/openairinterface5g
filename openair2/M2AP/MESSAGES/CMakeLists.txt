set(M2AP_VERSION 14 0 0)
make_version(M2AP_cc ${M2AP_VERSION})
string(REPLACE ";" "." M2AP_RELEASE "${M2AP_VERSION}")

if(M2AP_RELEASE VERSION_EQUAL "16.3.1")
  include(ASN1/m2ap-16.3.1.cmake)
elseif(M2AP_RELEASE VERSION_EQUAL "8.9.0")
  include(ASN1/m2ap-8.9.0.cmake)
elseif(M2AP_RELEASE VERSION_EQUAL "11.9.0")
  include(ASN1/m2ap-11.9.0.cmake)
elseif(M2AP_RELEASE VERSION_EQUAL "12.9.0")
  include(ASN1/m2ap-12.9.0.cmake)
elseif(M2AP_RELEASE VERSION_EQUAL "14.0.0")
  include(ASN1/m2ap-14.0.0.cmake)
elseif(M2AP_RELEASE VERSION_EQUAL "15.1.0")
  include(ASN1/m2ap-15.1.0.cmake)
else()
  message(FATAL_ERROR "unknown M2AP_RELEASE ${M2AP_RELEASE}")
endif()

add_custom_command(OUTPUT ${m2ap_source} ${m2ap_headers}
  COMMAND ASN1C_PREFIX=M2AP_ ${ASN1C_EXEC} -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${M2AP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${M2AP_GRAMMAR}
  COMMENT "Generating M2AP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${M2AP_GRAMMAR}"
)

add_library(asn1_m2ap ${m2ap_source})
target_include_directories(asn1_m2ap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_m2ap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
