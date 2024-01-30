<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI T1/T2 LDPC offload</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

[[_TOC_]]

This documentation aims to provide a tutorial for AMD Xilinx T2 Telco card integration into OAI and its usage.

# Requirements

 - bitstream image and PMD driver for the T2 card provided by AccelerComm
 - DPDK 20.11.9 with patch from Accelercomm: version ACCL_BBDEV_DPDK20.11.3_ldpc_3.1.918.patch
 - tested on RHEL7.9, RHEL9.2, Ubuntu 22.04

# DPDK setup
## DPDK installation
*Note: Following instructions are valid for ACCL_BBDEV_DPDK20.11.3_ldpc_3.1.918.patch version, which is compatible with DPDK 20.11.9. Installation steps, which should be followed for older versions of the patch file (for example ACL_BBDEV_DPDK20.11.3_BL_1006_build_1105_dev_branch_MCT_optimisations_1106_physical_std.patch) are present in older version of this documentation, under the tag 2023.w48.*

```
# Get DPDK source code
git clone https://github.com/DPDK/dpdk-stable.git ~/dpdk-stable
cd ~/dpdk-stable
git checkout v20.11.9
git apply ~/ACL_BBDEV_DPDK20.11.3_ldpc_3.1.918.patch
```
Replace `~/ACL_BBDEV_DPDK20.11.3_ldpc_3.1.918.patch` by patch file provided by
Accelercomm.
```
cd ~/dpdk-stable
meson setup build
# meson setup --prefix=/opt/dpdk-t2 build for installation with non-default installation prefix
cd build
ninja
sudo ninja install
sudo ldconfig
```
## DPDK configuration
 - load required kernel module
```
sudo modprobe vfio-pci
```
 - check presence of the card and its PCI addres on the host machine
```
lspci | grep "Xilinx"
```
 - bind the card with vfio-pci driver
```
sudo python3 ~/dpdk-stable/usertools/dpdk-devbind.py --bind=vfio-pci 41:00.0
```
Replace PCI address of the card *41:00.0* by address detected by *lspci | grep "Xilinx"* command
 - hugepages setup (10 x 1GB hugepages)
```
sudo python3 ~/dpdk-stable/usertools/dpdk-hugepages.py -p 1G --setup 10G
```

*Note: device binding and hugepages setup has to be done after every reboot of
the host machine*

# Modifications in the OAI code
## DPDK lib and PMD path specification
Path to the DPDK lib and Accelercomm PMD for operating the card is specified in the `CMakeLists.txt` file, in
*LDPC OFFLOAD library* section. Modify following line based on the location of
`libdpdk.pc` file associated with the target DPDK library on your system. By default, the path is set to `/usr/local/lib/x86_64-linux-gnu/pkgconfig`.
```
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/x86_64-linux-gnu/pkgconfig")
```

## T2 card DPDK initialization
Following lines in `openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_offload.c` file has to be
modified based on your system requirements. By default, PCI address of the T2 card is set to 41:00.0 and cores 14 and 15 are assigned to the DPDK.
```
 char *dpdk_dev = "41:00.0"; //PCI address of the card
 char *argv_re[] = {"bbdev", "-a", dpdk_dev, "-l", "14-15", "--file-prefix=b6", "--"};
```
For the DPDK EAL initialization, device is specified by `-a` option and list
of cores to run the DPDK application on is selected by `-l` option. PCI adress of
the T2 card can be detected by `lspci | grep "Xilinx"` command.

# OAI Build
OTA deployment is precisely described in the following tutorial:
- [NR_SA_Tutorial_COTS_UE](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/doc/NR_SA_Tutorial_COTS_UE.md)
Instead of section *3.2 Build OAI gNB* from the tutorial, run following commands:

```
# Get openairinterface5g source code
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g
git checkout develop

# Install OAI dependencies
cd ~/openairinterface5g/cmake_targets
./build_oai -I

# Build OAI gNB
cd ~/openairinterface5g
source oaienv
cd cmake_targets
./build_oai -w USRP --ninja --gNB -P --build-lib "ldpc_t2" -C
```
Shared object file *libldpc_t2.so* is created during the compilation. This object is conditionally compiled. Selection of the library to compile is done using *--build-lib ldpc_t2*.

*Required poll mode driver has to be present on the host machine and required DPDK version has to be installed on the host, prior to the build of OAI*

# 5G PHY simulators
## nr_ulsim test
Offload of the channel decoding to the T2 card is in nr_ulsim specified by *-o* option. Example command for running nr_ulsim with LDPC decoding offload to the T2 card:
```
cd ~/openairinterface5g
source oaienv
cd cmake_targets/ran_build/build
sudo ./nr_ulsim -n100 -s20 -m20 -r273 -R273 -o
```
## nr_dlsim test
Offload of the channel encoding to the AMD Xilinx T2 card is in nr_dlsim specified by *-c* option. Example command for running nr_dlsim with LDPC encoding offload to the T2 card:
```
cd ~/openairinterface5g
source oaienv
cd cmake_targets/ran_build/build
sudo ./nr_dlsim -n300 -s30 -R 106 -e 27 -c
```

# OTA test
Offload of the channel encoding and decoding to the AMD Xilinx T2 card is enabled by *--ldpc-offload-enable* option.

## Run OAI gNB with USRP B210
```
cd ~/openairinterface5g
source oaienv
cd cmake_targets/ran_build/build
sudo ./nr-softmodem --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --ldpc-offload-enable
```

# Limitations
## AMD Xilinx T2 card
 - functionality of the LDPC encoding and decoding offload verified in OTA SISO setup with USRP N310 and Quectel RM500Q, blocking of the card reported for MIMO setup (2 layers)

*Note: AMD Xilinx T1 Telco card is not supported anymore.*
