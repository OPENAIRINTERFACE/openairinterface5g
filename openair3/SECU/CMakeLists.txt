# SPDX-License-Identifier: LicenseRef-CSSL-1.0

pkg_check_modules(OpenSSL REQUIRED IMPORTED_TARGET openssl)
add_library(SECURITY
  secu_defs.c
  curve_25519.c
  x963_kdf.c
  kdf.c
  aes_128_ctr.c
  aes_128_cbc_cmac.c
  aes_128_ecb.c
  sha_256_hmac.c
  rijndael.c
  snow3g.c
  key_nas_deriver.c
  nas_stream_eea0.c
  nas_stream_eea1.c
  nas_stream_eia1.c
  nas_stream_eea2.c
  nas_stream_eia2.c
)
target_include_directories(SECURITY PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SECURITY PRIVATE PkgConfig::OpenSSL ds)
