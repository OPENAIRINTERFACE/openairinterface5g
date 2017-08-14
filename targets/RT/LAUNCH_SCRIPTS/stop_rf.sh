killall lte_scope
killall lte_scope_ul
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 4 1
sudo rmmod -f nasmesh
sudo rmmod -f openair_rf
sudo rmmod -f asn1_msg_kern

