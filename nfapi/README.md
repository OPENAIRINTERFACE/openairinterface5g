# Changelog

### 14/06/20
* we are not clear on how the parameters are extracted from the config file
* in ssc struct
* for some values it is okay and for others it is zero (in `config_request`)
* This causes Assertion Failures leading to termination on PNF side.

### 15/06/20
* The values to config request are assigned in: `config_common()` in `config.c`
* need to add lines for `cfg->nfapi_config`
* the config request unpacked on pnf side still has some zero values, which causes assertional failure in from_nrarfcn()

### 16/06/20
* Config request is recived and unpacked correctly
* Regarding config response
    * we are confused about its' struct definition
    * it says list of TLVs in its entirety
    * and we need to dyanmically insert the invalid TLVs after checking in a categorical fashion
    * Need to think of a data structure to enable this feature.
    ```struct{
    list of uint8_t tlvs
    list of uint16_t tlvs
    list of uint32_t tlvs
    }list of tlvs
    ```
* Need the `N_RB` value to be non-zero and obtained from the config file. (carrier config grid size)
`int N_RB = gNB_config->carrier_config.dl_grid_size[gNB_config->ssb_config.scs_common.value].value;`
* how is this `gNB_config` assigned its values?

### 19/06/20
* `ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);` sloc: 1428 in `nr-ru.c`
* the `ru->rfdevice` has all members zero or null.
* This issue was solved by adding `sdr_addrs` to config file of PNF.