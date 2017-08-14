#!/bin/sh
echo "start setting up eNB interface"

cd $OPENAIR2_DIR
sudo rmmod nasmesh
#make nasmesh_netlink_address_fix.ko
#make nasmesh_netlink.ko
#make nasmesh.ko
#make rb_tool
#route add -net 224.0.0.0 netmask 240.0.0.0 dev eth0
cd -

sudo insmod $OPENAIR2_DIR/NAS/DRIVER/MESH/nasmesh.ko 

sudo  ifconfig oai0 10.0.1.1 netmask 255.255.255.0 broadcast 10.0.1.255

$OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL/rb_tool -a -c0 -i0 -z0 -s 10.0.1.1 -t 10.0.1.2 -r 3

$OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL/rb_tool -a -c1 -i0 -z0 -s 10.0.1.1 -t 10.0.1.3 -r 11

echo "end setting up NAS interface"
