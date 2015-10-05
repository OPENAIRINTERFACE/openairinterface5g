####################################################################################
# Makefile for iptables extension for the GTPUSP target and GTPUSP klm
#
# Copyright (c) 2010-2011 Polaris Networks
# Author: Pradip Biswas <pradip_biswas@polarisnetworks.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
####################################################################################

export BIN_DIR ?= $(shell pwd)/Bin
KVERSION = $(shell uname -r)
KVERSION_LAST_DIGIT = $(shell echo ${KVERSION} | cut -d "." -f 3 | cut -d "-" -f 1)

obj-m = xt_GTPUSP.o
EXTRA_CFLAGS += -DKVERSION=$(KVERSION_LAST_DIGIT)

all: xtlib
	echo "Building for Kernel Version: $(KVERSION_LAST_DIGIT)"
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	chmod 777 xt_GTPUSP.ko
	mv xt_GTPUSP.ko $(BIN_DIR)/

clean: libclean
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

################################################################################
# The next section is used by libxt_GTPUSP
################################################################################

XT_IPTABLES_VERSION = $(shell /sbin/iptables -V | cut -d " " -f 2 | cut -b 2-)
IPT_FIRST_NUM = $(shell echo ${XT_IPTABLES_VERSION} | cut -d "." -f 1 | cut -d "-" -f 1)
IPT_SECOND_NUM = $(shell echo ${XT_IPTABLES_VERSION} | cut -d "." -f 2 | cut -d "-" -f 1)
IPT_THIRD_NUM = $(shell echo ${XT_IPTABLES_VERSION} | cut -d "." -f 3 | cut -d "-" -f 1)
IPTVERSION = $(IPT_FIRST_NUM)$(IPT_SECOND_NUM)$(IPT_THIRD_NUM)
IPTABLES_VERSION_STRING = \"$(IPT_FIRST_NUM).$(IPT_SECOND_NUM).$(IPT_THIRD_NUM)\"

LIB_CC := gcc

ifeq ($(IPTVERSION), 135)
    LIB_LD := ld 
else
    LIB_LD := gcc 
endif

LIB_CFLAGS := -fPIC -Wall -MMD -D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 \
          -D_REENTRANT -pipe -DXTABLES_LIBDIR=\"/usr/local/libexec/xtables\" -DXTABLES_INTERNAL \
          -D_INIT=libxt_GTPUSP_init -DPIC -fPIC -O2 \
          -DIPTVERSION=$(IPTVERSION) \
          -DIPTABLES_VERSION_STRING=$(IPTABLES_VERSION_STRING)

XT_TARGET := xt_GTPUSP
XT_LIBNAME := lib$(XT_TARGET).so
XT_BUILD_DIR = $(BIN_DIR)/$(XT_TARGET)

# Get the system distribution type
IsDebian=$(shell /bin/cat /proc/version | grep -i -e ubuntu -e debian)
IsRedHat=$(shell /bin/cat /proc/version | grep -i -e centos -e "red hat" -e suse -e fedora) 

# Change the ipq library name for 64-bit Debian-based distributions
ifeq ($(X86_64),x86_64)
    ifneq ($(strip $(IsDebian)),)
        LIB_IPQ=_pic
    endif
endif

INCLUDE_DIR = -I. \
              -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include \
              -I/usr/lib64/glib-2.0/include

XT_SOURCES = libxt_GTPUSP.c

XT_OBJECTS := $(addprefix $(XT_BUILD_DIR)/, $(notdir $(XT_SOURCES:.c=.o)))

xtlib: XT

XT: DF = $(XT_BUILD_DIR)/$(*F)
XT: $(XT_BUILD_DIR) $(BIN_DIR)/$(XT_LIBNAME)

-include $(addprefix $(XT_BUILD_DIR)/, $(notdir $(XT_SOURCES:.c=.P)))

$(XT_BUILD_DIR):
	@echo Build Directory is $@
	@if [ ! -d $(BIN_DIR) ]; then mkdir $(BIN_DIR); fi
	@if [ ! -d $@ ]; then mkdir $@; fi

$(BIN_DIR)/$(XT_LIBNAME): $(XT_OBJECTS)
	@echo Linking $@
	@$(LIB_LD) -shared $(XT_OBJECTS) -o $@

$(XT_BUILD_DIR)/%.o : %.c
	@echo "Building GTPUSP ext for version $(IPTVERSION) $(IPTABLES_VERSION_STRING)"
	@echo Compiling $<
	@$(LIB_CC) $(LIB_CFLAGS) -MF $(DF).d $(INCLUDE_DIR) -o $@ -c $<
	@cp $(DF).d $(DF).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DF).d >> $(DF).P; \
	rm -f $(DF).d

libclean:
	@rm -rf $(BIN_DIR)/$(XT_LIBNAME) $(XT_BUILD_DIR)

