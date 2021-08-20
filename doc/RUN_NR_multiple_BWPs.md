# Procedure to add dedicated Bandwidth part (BWP)

## Contributed by 5G Testbed IISc 

### Developers: Abhijith A, Shruthi S

# Terminology #

## Bandwidth part (BWP) ##
Bandwidth Part (BWP) is a set of contiguous Resource Blocks in the resource grid. 

Parameters of a Bandwidth Part are communicated to UE using RRC parameters: BWP-Downlink and BWP-Uplink. 

A UE can be configured with a set of 4 BWPs in uplink (UL) and downlink (DL) direction. But only 1 BWP can be active in UL and DL direction at a given time.

# Procedure to run multiple dedicated BWPs #

A maximum of 4 dedicated BWPs can be configured for a UE.

To configure multiple BWPs, add the following parameters to the gNB configuration file under "servingCellConfigDedicated": 

## Setup of the Configuration files ##
```
    firstActiveDownlinkBWP-Id = 1; #BWP-Id 
    defaultDownlinkBWP-Id = 1; #BWP-Id 
    firstActiveUplinkBWP-Id = 1; #BWP-Id 
```

Each dedicated BWP must have:
```
    # BWP 1 Configuration 
    bwp-Id_1 = 1; 
    bwp1_locationAndBandwidth = 28908; // RBstart=33,L=106 
    # subcarrierSpacing 
    # 0=kHz15, 1=kHz30, 2=kHz60, 3=kHz120  
    bwp1_subcarrierSpacing = 1; 
```   

 Find these parameters in this configuration file: "targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.SCD.conf"

###  NOTE:  
    * For changing bandwidth, 'dl_carrierBandwidth' and 'ul_carrierBandwidth' must be modified according 
      to the bandwidth of the BWP
    * Currently only 'firstActiveDownlinkBWP-Id' is used to configure both DL and UL BWPs
     
# Testing gNB and UE in RF simulator

## gNB command:
```
    sudo RFSIMULATOR=server ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.SCD.conf --rfsim --phy-test -d
```

## UE command:
```
    sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --rfsim --phy-test -d
```