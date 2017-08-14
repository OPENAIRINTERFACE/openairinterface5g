sudo sh $OPENAIR1_DIR/xio_script.sh
sh stop_rf.sh
sh start_rf.sh

$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 0 0 1 # dump Config
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 6 110 110 100 100 #Set TX gains
#$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 2 26 100   #Card v2_10 synched to v2_19
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 14 0  #SET RX RF Mode
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 32 1  #Set DLSCH Transmission Mod
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 1 0 0 #Start Primary Clusterhead

#$OPENAIR1_DIR/USERSPACE_TOOLS/SCOPE/lte_scope_ul 0 0 &
#$OPENAIR1_DIR/USERSPACE_TOOLS/SCOPE/lte_scope_ul 0 1 &