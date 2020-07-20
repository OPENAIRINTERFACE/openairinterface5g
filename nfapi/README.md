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

# 20 July 2020
## Plan

### Task-A [packing]
* Modify `nfapi_p7_message_pack()` in `nfapi_p7.c`
* In the switch case, change the labels as well as the pack functions:
    * `pack_dl_config_request` becomes `pack_dl_tti_request`
    * `pack_ul_config_request` becomes `pack_ul_tti_request`
    * `pack_hi_dci0_request` becomes `pack_ul_dci_request`
    * `pack_tx_request` becomes `pack_tx_data_request`

### Task-B [unpacking]
* Modify `nfapi_p7_message_unpack()` in `nfapi_p7.c`
* Similarly change all the `unpack` functions within the switch-case block.
* Modify the behaviour of `check_unpack_length()` for the new `TAGS`

### Task-C
* Write the `dl_tti` alternative for `nfapi_vnf_p7_dl_config_req()` in `vnf_p7_interface.c` and other such functions.
* Also check the function `vnf_p7_pack_and_send_p7_msg()` in `vnf_p7.c` for upgrade.
* Write the `dl_tti` equivalent for `oai_nfapi_dl_config_req` in `nfapi_vnf.c` and other such functions.
* Check if `nr_schedule_response()` needs to be upgraded

### Testing
* Test by running VNF on a terminal and PNF in rfsim parallely
* If all works fine, move to UE testing.
* Check if the `FAPI` core functionality is not broken by running gNB in monolithic mode
* Syncwith the latest stable commit in `develop` branch

* Testing with a UE
    * Run VNF
    * Run PNF in rfsim
    * Run UE in rfsim
* This can be done on parallel terminals on the same machine or different machine with the appropriate configuration