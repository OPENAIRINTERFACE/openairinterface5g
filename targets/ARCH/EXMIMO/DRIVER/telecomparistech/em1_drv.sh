#!/bin/sh

sudo insmod em1.ko
sudo mknod /dev/exmimo1 c 256 0
sudo chmod 666 /dev/exmimo1
