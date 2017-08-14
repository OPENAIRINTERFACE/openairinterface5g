gpsd /dev/ttyUSB0
cd USERSPACE_TOOLS/OPENAIR_RF

./openair_rf_cbmimo1_lte 0 4 1
rmmod -f openair_rf
cd ../..
make install_cbmimo1_softmodem_lte_emos_l2
cd EMOS/LTE/GUI
./emos_gui &

