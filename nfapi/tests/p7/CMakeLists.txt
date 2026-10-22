# SPDX-License-Identifier: LicenseRef-CSSL-1.0

set(Test_Labels fapi p7)
set(_fapi_p7_messages "dci_inversion;dl_tti_request;ul_tti_request;slot_indication;ul_dci_request;tx_data_request;rx_data_indication;crc_indication;uci_indication;srs_indication;rach_indication;srs_toa_vendor_ext_indication")
foreach (fapi_p7_message IN LISTS _fapi_p7_messages)
    add_executable(nr_fapi_${fapi_p7_message}_test nr_fapi_${fapi_p7_message}_test.c)
    target_link_libraries(nr_fapi_${fapi_p7_message}_test PUBLIC nr_fapi_p7)
    target_link_libraries(nr_fapi_${fapi_p7_message}_test PRIVATE pthread UTIL ${T_LIB} minimal_lib)
    add_dependencies(tests nr_fapi_${fapi_p7_message}_test)

    add_test(nr_fapi_${fapi_p7_message}_test nr_fapi_${fapi_p7_message}_test)
    set_tests_properties(nr_fapi_${fapi_p7_message}_test PROPERTIES LABELS "${Test_Labels}")
endforeach ()
add_executable(nr_fapi_p7_phy_id_test nr_fapi_p7_phy_id_test.c)
target_link_libraries(nr_fapi_p7_phy_id_test PUBLIC nr_fapi_p7)
target_link_libraries(nr_fapi_p7_phy_id_test PRIVATE pthread UTIL ${T_LIB} minimal_lib)
add_dependencies(tests nr_fapi_p7_phy_id_test)

add_test(nr_fapi_p7_phy_id_test nr_fapi_p7_phy_id_test)
set_tests_properties(nr_fapi_p7_phy_id_test PROPERTIES LABELS "${Test_Labels}")

add_executable(nr_fapi_p7_bounds_test nr_fapi_p7_bounds_test.c)
target_link_libraries(nr_fapi_p7_bounds_test PUBLIC nr_fapi_p7)
target_link_libraries(nr_fapi_p7_bounds_test PRIVATE pthread UTIL ${T_LIB} minimal_lib)
add_dependencies(tests nr_fapi_p7_bounds_test)

add_test(nr_fapi_p7_bounds_test nr_fapi_p7_bounds_test)
set_tests_properties(nr_fapi_p7_bounds_test PROPERTIES LABELS "${Test_Labels}")

# Add the dci label for the dci payload test
set(dci_labels dci ${Test_Labels})
set_tests_properties(nr_fapi_dci_inversion_test PROPERTIES LABELS "dci ${Test_Labels}")
# Add the FAPI_BYTE_ORDERING_BIG_ENDIAN preprocessor define in order to test the invert/uninvert dci payload functions
target_compile_definitions(nfapi_common PRIVATE FAPI_BYTE_ORDERING_BIG_ENDIAN)
