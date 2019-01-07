cd USERSPACE_TOOLS/OPENAIR_RF
./openair_rf_cbmimo1_lte 0 4 1
rmmod -f openair_rf
cd ../..
make install_cbmimo1_softmodem_lte_emos_l2
cd USERSPACE_TOOLS/OPENAIR_RF
./openair_rf_cbmimo1_lte 0 0 1 1
./openair_rf_cbmimo1_lte 0 6 137 137 140 140
./openair_rf_cbmimo1_lte 0 14 2
./openair_rf_cbmimo1_lte 0 32 6
source /opt/XIO2000/xio_script.sh
./openair_rf_cbmimo1_lte 0 1 1 0
cd ../..
