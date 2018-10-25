Common command line:
--------------------
cd /openairinterface5g/
source oaienv

HWLAT application:
------------------
./cmake_targets/build_oai -c -C -w ADRV9371_ZC706 --HWLAT
./cmake_targets/lte-hwlat/build/lte-hwlat

LTE-SOFTMODEM application:
--------------------------
./cmake_targets/build_oai -c --eNB --UE --noS1 -w ADRV9371_ZC706
sudo su
source oaienv
source ./targets/bin/init_nas_nos1 UE
./cmake_targets/lte_noS1_build_oai/build/lte-softmodem-nos1 -U -C 2680000000 -r100 --ue-scan-carrier --ue-txgain 0 --ue-rxgain 5 -S -A 6 --ue-max-power -25 --phy-test -g 7 --rf-config-file ./targets/ARCH/ADRV9371_ZC706/USERSPACE/PROFILES/ue.band7.tm1.PRB100.adrv9371-zc706_HWgain15dB.ini
