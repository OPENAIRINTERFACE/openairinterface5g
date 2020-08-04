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
    * `pack_dl_config_request` becomes `pack_dl_tti_request`    [x]
    * `pack_ul_config_request` becomes `pack_ul_tti_request`    [x]
    * `pack_hi_dci0_request` becomes `pack_ul_dci_request`      [x]
    * `pack_tx_request` becomes `pack_tx_data_request`          [x]

### Task-B [unpacking]
* Modify `nfapi_p7_message_unpack()` in `nfapi_p7.c`
    * `unpack_dl_tti_request`   [x]
    * `unpack_ul_tti_request`   [x]
    * `unpack_ul_dci_request`   [x]
    * `unpack_tx_data_request`  [x]

* Similarly change all the `unpack` functions within the switch-case block.

* Modify the behaviour of `check_unpack_length()` for the new `TAGS`
    * this is where the unpack functions are called
        * `dl_tti_request`  [x]
        * `ul_tti_request`  [x]
        * `ul_dci_request`  [x]
        * `tx_data_request` [x]
* `nfapi_p7_message_unpack()` is called in `pnf_handle_dl_config_request()` in pnf_p7.c, so we need to add 
    * `pnf_handle_dl_tti_request`
    * `pnf_handle_ul_tti_request`
    * `pnf_handle_ul_dci_request`
    * `pnf_handle_tx_data_request`
    to handle DL P7 messages at pnf
### Task-C
* Write the `ul_tti` alternative for `nfapi_vnf_p7_ul_config_req()` in `vnf_p7_interface.c` and other such functions.
    * `dl_tti` is present as `nr_dl_config` [x]
    * `ul_tti`  [x]
    * `ul_dci`  [x]
    * `tx_data` [x]

* Resolve the hard-coded areas in source code. [`HIGH-PRIORITY`] [x]

* Write the `ul_tti` equivalent for `oai_nfapi_dl_config_req` in `nfapi_vnf.c` and other such functions.
    * `dl_tti` is present as `nr_dl_config`             [x]
    * `ul_tti` is present as `oai_nfapi_ul_tti_req()`   [x]
    * `ul_dci` is present as `oai_nfapi_ul_dci_req()`   [x]
    * `tx_data` is present as `oai_nfapi_tx_data_req()` [x]

* Check if `nr_schedule_response()` needs to be upgraded [x]
    * only a couple of `NFAPI_MODE!=VNF_MODE` checks *can be added* before calling the functions to handle PDUs
    * we need to add `NFAPI_MODE!=MONOLITHIC` checks before `oai_nfapi_dl_config_req` and other such fns [x]
* Sync with the latest stable commit in `develop` branch [`LOW-PRIORITY`]
    * check deviations


### Testing
* Test by running VNF on a terminal and PNF in rfsim parallely
* If all works fine, move to UE testing.
* Check if the `FAPI` core functionality is not broken by running gNB in monolithic mode
* Check whether the code can support latency of 0.5 ms for oai_slot_indication

* Testing with a UE
    * Run VNF
    * Run PNF in rfsim
    * Run UE in rfsim
* This can be done on parallel terminals on the same machine or different machine with the appropriate configuration