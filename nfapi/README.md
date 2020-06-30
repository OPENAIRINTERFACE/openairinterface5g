## Running using USRP
### PNF
```
sudo <oai_codebase>/cmake_targets/ran_build/build/nr-softmodem -O <oai_codebase>/targets/PROJECTS/GENERIC-LTE-EPC/CONF/oaiL1.nfapi.usrpx300.conf --pnf
```

### VNF
```
sudo <oai_codebase>/cmake_targets/ran_build/build/nr-softmodem -O <oai_codebase>/targets/PROJECTS/GENERIC-LTE-EPC/CONF/rcc.band78.tm1.106PRB.nfapi.conf --vnf 
```


## Running using RF-Simulator
### PNF
```
sudo RFSIMULATOR=server <oai_codebase>/cmake_targets/ran_build/build/nr-softmodem -O <oai_codebase>/targets/PROJECTS/GENERIC-LTE-EPC/CONF/oaiL1.nfapi.usrpx300.conf --pnf --parallel-config PARALLEL_SINGLE_THREAD --rfsim
```
### VNF
```
sudo <oai_codebase>/cmake_targets/ran_build/build/nr-softmodem -O <oai_codebase>/targets/PROJECTS/GENERIC-LTE-EPC/CONF/rcc.band78.tm1.106PRB.nfapi.conf --vnf
```