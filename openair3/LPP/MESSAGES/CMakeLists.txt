#set(LPP_VERSION 16 78 0)
#make_version(LPP_cc ${LPP_VERSION})
#add_definitions(-DLPP_VERSION=${LPP_cc})
#string(REPLACE ";" "." LPP_RELEASE "${LPP_VERSION}")

#if(LPP_RELEASE VERSION_EQUAL "16.78.0")
include(ASN1/37355-g60.cmake)
#else()
#  message(FATAL_ERROR "unknown LPP_RELEASE ${LPP_RELEASE}")
#endif()

add_custom_command(OUTPUT ${lpp_source} ${lpp_headers}
  COMMAND ASN1C_PREFIX=LPP_ ${ASN1C_EXEC} -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${LPP_GRAMMAR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LPP_GRAMMAR}
  COMMENT "Generating LPP source files from ${CMAKE_CURRENT_SOURCE_DIR}/${LPP_GRAMMAR}"
)

add_library(asn1_lpp ${lpp_source})
target_include_directories(asn1_lpp PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_lpp PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
