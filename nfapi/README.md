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