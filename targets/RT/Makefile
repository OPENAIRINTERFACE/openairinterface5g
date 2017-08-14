# command line flags are automatically exported and thus passed down to any sub-make. 
# here we set the default flags in case on command line flags are provided
# these flags have to be explicitely exported so that the sub-make is aware of them


include $(OPENAIR_DIR)/common/utils/Makefile.inc
ifndef OPENAIR_LTE
export OPENAIR_LTE=1
export OPENAIR_EMU=0
endif

ifndef OPENAIR1
export OPENAIR1=0
endif

ifndef OPENAIR2
export OPENAIR2=0
endif

ifndef OPENAIR3
export OPENAIR3=0
endif

ifndef EMOS
export EMOS=0
endif

ifndef rrc_cellular
export rrc_cellular = 0
export rrc_cellular_eNB = 0
export rrc_cellular_UE = 0
else
 export rrc_cellular = 1	
  ifeq ($(eNB_flag),1)
    export rrc_cellular_eNB=1
  endif
  ifeq ($(UE_flag),1)
    export rrc_cellular_UE=1
  endif
endif

all: openair_rf_cbmimo1_softmodem.ko asn1_msg_kern.ko nasmesh.ko rb_tool openair_rf scope

openair_rf_cbmimo1_softmodem.ko:  
	(cd $(OPENAIR1_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 && mv openair_rf.ko $(CURDIR)/openair_rf_softmodem.ko)

oai_user_cbmimo1.ko:
	(cd $(OPENAIR1_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 BIT8_TX=1 OPENAIR1=0 OPENAIR2=0 && mv openair_rf.ko $(CURDIR)/openair_rf_softmodem.ko)

oai_user_exmimo.ko:
	(cd $(OPENAIR1_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) -C $(KERNEL_DIR)/build V=1 M=`pwd` RTAI=1 CBMIMO1=1 OPENAIR1=0 OPENAIR2=0 && mv openair_rf.ko $(CURDIR)/openair_rf_softmodem.ko)

asn1_msg_kern.ko:
	(cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES && $(MAKE) -C $(KERNEL_DIR)/build V=0 M=`pwd` RTAI=1 -j2 && mv asn1_msg_kern.ko $(CURDIR)/asn1_msg_kern.ko)

nasmesh.ko:
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build M=`pwd` && mv nasmesh.ko $(CURDIR)/nasmesh.ko)

rb_tool:
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH/RB_TOOL && $(MAKE))

openair_rf:
	(cd $(OPENAIR1_DIR)/USERSPACE_TOOLS/OPENAIR_RF && $(MAKE) clean CBMIMO1=1 && $(MAKE) all CBMIMO1=1 && $(MAKE) clean CBMIMO1=1)

scope:
	(cd $(OPENAIR1_DIR)/USERSPACE_TOOLS/SCOPE && $(MAKE) clean && $(MAKE) all && $(MAKE) clean )	

#Remove all but source files
cleanall: 
	(cd $(OPENAIR1_DIR) && $(MAKE) cleanall)
	(cd $(OPENAIR2_DIR) && $(MAKE) cleanall)

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
	insmod openair_rf_softmodem.ko

updatefw:
	$(OPENAIR1_DIR)/USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 

boot_exmimo:
	sudo make install_oai_user
	$(OPENAIR1_DIR)/USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 
	sleep 1
	sudo rmmod openair_rf
	sudo make install_oai_user

reboot_exmimo:
	$(OPENAIR1_DIR)/USERSPACE_TOOLS/OAI_FW_INIT/updatefw -f $$OPENAIR0_DIR/express-mimo/software/sdr/main -s 0x43fffff0 -b
	sudo rmmod openair_rf
	sudo make install_oai_user

install_softmodem:
	make fifos
ifeq ($(OPENAIR2),1)
	insmod asn1_msg_kern.ko
endif
	insmod openair_rf_softmodem.ko

remove:
	rmmod openair_rf
ifeq ($(OPENAIR2),1)
	rmmod asn1_msg_kern
endif

test:
	(cd $(OPENAIR1_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) test RTAI=1 CBMIMO1=1)	
	(cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES && $(MAKE) test)

clean:
	(cd $(OPENAIR1_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER && $(MAKE) clean RTAI=1 CBMIMO1=1)
	(cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES && $(MAKE) clean)
