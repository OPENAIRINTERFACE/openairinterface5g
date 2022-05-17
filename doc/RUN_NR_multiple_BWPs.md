# Procedure to add dedicated Bandwidth part (BWP)

## Contributed by 5G Testbed IISc 

### Developers: Abhijith A, Shruthi S

# Terminology #

## Bandwidth part (BWP) ##
Bandwidth Part (BWP) is a set of contiguous Resource Blocks in the resource grid. 

Parameters of a BWP are communicated to UE using RRC parameters: BWP-Downlink and BWP-Uplink. 

A UE can be configured with a set of 4 BWPs in uplink (UL) and downlink (DL) direction (3GPP TS 38.331 Annex B.2 Description of BWP configuration options). But only 1 BWP can be active in UL and DL direction at a given time.

# Procedure to run multiple dedicated BWPs #

A maximum of 4 dedicated BWPs can be configured for a UE.

To configure multiple BWPs, add the following parameters to the gNB configuration file under "servingCellConfigDedicated": 

## Setup of the Configuration files ##
```
    firstActiveDownlinkBWP-Id = 1;  #BWP-Id
    defaultDownlinkBWP-Id     = 1;  #BWP-Id
    firstActiveUplinkBWP-Id   = 1;  #BWP-Id
```

Each dedicated BWP must have:
```
    # BWP 1 Configuration
    dl_bwp-Id_1 = 1;
    dl_bwp1_locationAndBandwidth = 28875; // RBstart=0, L=106 (40 MHz BW)
    # subcarrierSpacing
    # 0=kHz15, 1=kHz30, 2=kHz60, 3=kHz120
    dl_bwp1_subcarrierSpacing = 1;
```   

 Find these parameters in this configuration file: "targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf"
     
# Testing gNB and UE in RF simulator

## gNB command:
```
    sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --gNBs.[0].min_rxtxtime 6 --rfsim --sa
```

## UE command:
```
    sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --nokrnmod --rfsim --sa --uicc0.imsi 208990000000001
```