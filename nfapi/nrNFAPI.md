# Procedure to run nFAPI in 5G NR

## Conributed by 5G Testbed IISc 

### Developers: Mahesh K,Gokul S,Aniq U R, Sai Shruthi N, Sudhakar B

## Procedure to Build gNB and UE

The regular commands to build gNB and UE can be used
```
sudo ./build_oai --gNB --nrUE

```
## Procedure to run NR nFAPI using RF-Simulator

### Bring up another loopback interface

If running for the first time on your computer, or you have restarted your computer, bring up another loopback interface with this command:  

sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up

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

To be updated.

## Notes
* If running for the first time on local branch, run in the following order - VNF then PNF. This is so that necessary files are generated for the UE to run. Stop this run (it's ok if it stops on its own due to an error). From every subsequent run follow the next bullet. 
* In order to achieve synchronization between the VNF and PNF, and receive P7 messages within the timing window, the order in which we should run the modules on different terminals is UE->VNF->PNF.
* Currently downlink transmission from gNB to UE is partially functional and we are working on improving this. Uplink P7 messages are disabled and we will work on uplink integration after completing downlink.
