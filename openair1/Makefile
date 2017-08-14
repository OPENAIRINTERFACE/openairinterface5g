# command line flags are automatically exported and thus passed down to any sub-make. 
# here we set the default flags in case on command line flags are provided
# these flags have to be explicitely exported so that the sub-make is aware of them


include $(OPENAIR_DIR)/common/utils/Makefile.inc
ifndef OPENAIR_LTE
export OPENAIR_LTE=1
endif

ifndef OPENAIR1
export OPENAIR1=1
endif

ifndef OPENAIR2
export OPENAIR2=0
endif

ifndef EMOS
export EMOS=0
endif

ifndef FIRMWARE2010
export FIRMWARE2010=0
endif

all: openair_rf_cbmimo1_softmodem.ko asn1_msg_kern.ko

openair_rf_cbmimo1_softmodem.ko:  
	(cd ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 && mv openair_rf.ko openair_rf_softmodem.ko)

oai_user_cbmimo1.ko:
	(cd ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 BIT8_TX=1 OPENAIR1=0 OPENAIR2=0 && mv openair_rf.ko openair_rf_softmodem.ko)

oai_user_exmimo.ko:
	(cd ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 OPENAIR1=0 OPENAIR2=0 && mv openair_rf.ko openair_rf_softmodem.ko)

asn1_msg_kern.ko:
	(cd ../openair2/RRC/LITE/MESSAGES && $(MAKE) -C $(KERNEL_DIR) V=0 M=`pwd` RTAI=1 -j2)

#Remove all but source files
cleanall: 
	find -name '*.o' -delete -print; \
	find -name '*.ko' -delete -print; \
	find -name '*.bak' -delete -print; \
	find -name '*.cmd' -delete -print; \
	find -name '*~' -delete -print; \
	find -name octave-core -delete -print; \

#Remove files created by SVN, dangerous!!! DO NOT RUN ON YOUR WORKING REPOSITORY!
#FK: Don't do this! Use svn export instead.
#cleansvn:
#	rm -rf `find -name *.svn* -print`; \
#	find -name *.svn* -delete -print

tgz:
	mkdir /tmp/openair1_tmp; \
	$(MAKE) cleanall; \
	cp -r . /tmp/openair1_tmp; \
	(cd /tmp/openair1_tmp && $(MAKE) cleansvn && cd .. && tar czfv openair1.tgz openair1_tmp) ; \
	cp /tmp/openair1.tgz . ;\
	rm -rf /tmp/openair1_tmp

fifos:
	@for i in `seq 0 64`;\
	do \
	have_rtfX=`ls /dev/ |grep -c rtf$$i`;\
	if [ "$$have_rtfX" -eq 0 ] ;then \
	mknod -m 666 /dev/rtf$$i c 150 $$i; \
	fi;\
	done

openair0:
	mknod /dev/openair0 c 127 0
	chmod a+rw /dev/openair0

install_oai_user:
	make fifos
	insmod ARCH/CBMIMO1/DEVICE_DRIVER/openair_rf_softmodem.ko

updatefw:
	USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr_exmimo2/main -s 0x43fffff0 

boot_exmimo:
	sudo make install_oai_user
	USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 
	sudo rmmod openair_rf
	sudo make install_oai_user

reboot_exmimo:
	USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 -b
	rmmod openair_rf
	make install_oai_user
	USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 
	rmmod openair_rf
	make install_oai_user

install_softmodem:
	make fifos
ifeq ($(OPENAIR2),1)
	insmod ../openair2/RRC/LITE/MESSAGES/asn1_msg_kern.ko
endif
	insmod ARCH/CBMIMO1/DEVICE_DRIVER/openair_rf_softmodem.ko ; \

remove:
	rmmod openair_rf
ifeq ($(OPENAIR2),1)
	rmmod asn1_msg_kern
endif

test:
	(cd ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) test RTAI=1 CBMIMO1=1)	
	(cd ../openair2/RRC/LITE/MESSAGES && $(MAKE) test)

clean: clean_l1 

clean_l1:
	(cd ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) clean RTAI=1 CBMIMO1=1)

clean_asn1:
	(cd ../openair2/RRC/LITE/MESSAGES && $(MAKE) clean)
