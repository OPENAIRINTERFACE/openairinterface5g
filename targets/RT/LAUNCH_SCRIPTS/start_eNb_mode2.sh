sudo sh $OPENAIR1_DIR/xio_script.sh
sh stop_rf.sh
sh start_rf.sh
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 0 1 1
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 6 110 110 100 100
#$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 2 26 100   #Card v2_10 synched to v2_19
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 14 0
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 32 2
$OPENAIR1_DIR/USERSPACE_TOOLS/OPENAIR_RF/openair_rf_cbmimo1_lte 0 1 1 0

