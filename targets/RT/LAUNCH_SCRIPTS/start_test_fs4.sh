sh $OPENAIR1_DIR/xio_script.sh
cd ../USERSPACE_TOOLS/OPENAIR_RF
./openair_rf_cbmimo1_lte 0 4 1
rmmod -f openair_rf
cd ../..
make install_cbmimo1_softmodem
cd USERSPACE_TOOLS/OPENAIR_RF
./openair_rf_cbmimo1_lte 0 0 1 1
./openair_rf_cbmimo1_lte 0 26 100
./openair_rf_cbmimo1_lte 0 6 150 150 140 140
./openair_rf_cbmimo1_lte 0 14 0
./openair_rf_cbmimo1_lte 0 8 1
cd ../../LAUNCH_SCRIPTS
