# Previous SYRIQ uninstall
#!/bin/sh
MODULE="riffa"
KERNEL_VERSION=$(uname -r)
RHR=/etc/redhat-release

sudo apt-get install -y linux-headers-`uname -r`

if lsmod | grep "$MODULE" &> /dev/null ; then
	echo "Previous SYRIQ is loaded!"
	sudo rmmod $MODULE
	echo " -> previous SYRIQ is unloaded!"
	sudo rm -f /usr/local/lib/libriffa.so*
	sudo rm -f /usr/local/include/riffa.h
	sudo rm -f /usr/local/include/riffa_driver.h
	sudo rm -f /etc/ld.so.conf.d/riffa.conf
	sudo rm -rf /lib/modules/$KERNEL_VERSION/kernel/drivers/riffa
	sudo rm -f /etc/udev/rules.d/99-riffa.rules
	sudo sed -i '/riffa/d' /etc/modules
	sudo ldconfig
	sudo depmod
	echo " -> previous SYRIQ is uninstall!"
	echo "Previous SYRIQ uninstallation is done"
	exit 0
else
	echo "Previous SYRIQ is not loaded!"
	exit 1
fi
