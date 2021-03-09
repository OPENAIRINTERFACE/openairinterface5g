# Procedure to run nFAPI in 5G NR

## Conributed by 5G Testbed IISC 
### Developers: Sudhakar B,Mahesh K,Gokul S,Aniq U.R

## Procedure to Build gNB and UE

The regular commands to build gNB and UE can be used
```
sudo ./build_oai --gNB --UE

```
## Procedure to run NR nFAPI using RF-Simulator

### VNF command
```
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/rcc.band78.tm1.106PRB.nfapi.conf --nfapi 2 --noS1 --phy-test

```
### PNF command
```
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/oaiL1.nfapi.usrpx300.conf --nfapi 1 --rfsim --phy-test --rfsimulator.serveraddr server

```
### UE command
```
sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --rfsim --phy-test --rrc_config_path . -d

```
## Procedure to run NR nFAPI using Hardware
Will be updated as we have not yet currently tested on hardware

## Notes
* In order to acheive the synchronization between VNF and PNF and receive the P7 messages within the timing window the order in which we should run the modules on different terminals is UE->VNF->PNF
* Currently only downlink is functional and working as we are still working on uplink functionality
