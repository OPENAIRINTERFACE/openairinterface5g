sudo rmmod openair_rf
sudo insmod $OPENAIR1_DIR/ARCH/CBMIMO1/DEVICE_DRIVER/openair_rf_softmodem.ko
sudo mknod /dev/openair0 c 127 0
sudo chmod a+rw /dev/openair0

$OPENAIR1_DIR/USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $OPENAIR1_DIR/USERSPACE_TOOLS/OAI_FW_INIT/main -s 0x43fffff0
sudo rmmod openair_rf
sudo insmod $OPENAIR1_DIR/ARCH/CBMIMO1/DEVICE_DRIVER/openair_rf_softmodem.ko

sudo rmmod nasmesh
sudo insmod $OPENAIR2_DIR/NAS/DRIVER/MESH/nasmesh.ko

