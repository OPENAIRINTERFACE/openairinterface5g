export KERNEL_DIR=/lib/modules/$(shell uname -r)/
CC=gcc
CCC=gcc
linux := $(shell if [ `uname` = "Linux" ] ; then echo "1" ; else echo "0" ; fi)


CFLAGS += -std=gnu99 
#CFLAGS += -Wall -g -ggdb -Wstrict-prototypes -fno-strict-aliasing 

# Need to force this option because default kernel module builder is wrong
CFLAGS += $(call cc-option,-mpreferred-stack-boundary=4)

#For performance, if some option doesn't exist in all gcc versions, use $(call cc-option,MY_OPTION)
CFLAGS += -O2
CFLAGS +=  -funroll-loops 
CFLAGS += -Wno-packed-bitfield-compat 

# This is the minimum CPU faetures for OAI
CFLAGS += -mmmx -msse -msse2 -mssse3 -msse4.1
# Add CPU features from local CPU
#CFLAGS += -march=native

ifeq ($(OSTYPE),Cygwin)
cygwin=1
CFLAGS += -DCYGWIN
else
cygwin=0
endif

ifeq ($(linux),1)
CFLAGS += 
LDFLAGS += 
endif

ITTI_DIR = $(COMMON_UTILS_DIR)/itti

ITTI_OBJS =  $(ITTI_DIR)/intertask_interface.o
ITTI_OBJS += $(ITTI_DIR)/intertask_interface_dump.o
ITTI_OBJS += $(ITTI_DIR)/backtrace.o
ITTI_OBJS += $(ITTI_DIR)/memory_pools.o
ITTI_OBJS += $(ITTI_DIR)/signals.o
ITTI_OBJS += $(ITTI_DIR)/timer.o


HASHTABLE_DIR = $(COMMON_UTILS_DIR)/collection/hashtable
MSC_DIR = $(COMMON_UTILS_DIR)/msc

HASHTABLE_OBJS =  $(HASHTABLE_DIR)/hashtable.o
HASHTABLE_OBJS += $(HASHTABLE_DIR)/obj_hashtable.o

UTILS_OBJS = $(ITTI_OBJS) $(HASHTABLE_OBJS)  

UTILS_incl = 				\
    -I$(COMMON_UTILS_DIR)   \
    -I$(HASHTABLE_DIR)      \
    -I$(ITTI_DIR) \
    -I$(MSC_DIR)

print-%  : ; @echo $* = $($*)
