# Procedure to run nFAPI in 5G NR

## Contributed by 5G Testbed IISc 

### Developers: Gokul S, Mahesh A, Aniq U R

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
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/rcc.band78.tm1.106PRB.nfapi.conf --nfapi VNF --noS1 --phy-test

```
### PNF command
```
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/oaiL1.nfapi.usrpx300.conf --nfapi PNF --rfsim --phy-test --rfsimulator.serveraddr server

```
### UE command
```
sudo ./nr-uesoftmodem --rfsim --phy-test -d --rfsimulator.serveraddr 127.0.0.1

```
## Procedure to run NR nFAPI using Hardware (tested with USRP x310)

### VNF command
```
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/rcc.band78.tm1.106PRB.nfapi.conf --nfapi VNF --noS1 --phy-test

```
### PNF command
```
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/oaiL1.nfapi.usrpx300.conf --nfapi PNF --phy-test

```
### UE command
```
sudo ./nr-uesoftmodem --usrp-args "addr=*USRP_ADDRESS*,clock_source=external,time_source=external" --phy-test

```

