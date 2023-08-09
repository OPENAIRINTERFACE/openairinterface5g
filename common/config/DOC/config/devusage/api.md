## config module management

```c
configmodule_interface_t *load_configmodule(int argc, char **argv, uint32_t initflags)
```
* Parses the command line options, looking for the –O argument
* Loads the `libparams_<configsource>.so` (today `libparams_libconfig.so`)  shared library
* Looks for `config_<config source>_init` symbol and calls it , passing it an array of string corresponding to the « : » separated strings used in the –O option
* Looks for `config_<config source>_get`, `config_<config source>_getlist` and  `config_<config source>_end` symbols which are the three functions a configuration library should implement. Get and getlist are mandatory, end is optional.
* Stores all the necessary information in a `configmodule_interface_t structure`, which is of no use for caller as long as we only use one configuration source.
*  if the bit CONFIG_ENABLECMDLINEONLY is set in `initflags` then the module allows parameters to be set only via the command line. This is used for the oai UE.

```c
void end_configmodule(void)
```
* Free memory which has been allocated by the configuration module for storing parameters values, when `PARAMFLAG_NOFREE` flag is not specified in the parameter definition.
* Possibly calls the `config_<config source>_end` function
This call should be used when all configurations have been read. The program will still be able to use parameter values allocated by the config module WHEN THE `PARAMFLAG_NOFREE` flag has been specified in the parameter definition. The config module can be reloaded later in the program (not fully tested as not used today)

## Retrieving parameter's values

```c
int config_get(paramdef_t *params,int numparams, char *prefix)
```
* Reads as many parameters as described in params, they must all be in the same configuration file section
* Calls the `config_<config source>_get` function
* Calls the `config_process_cmdline` function
* `params` points to an array of `paramdef_t` structures which describes the parameters to be read, possibly including a pointer to a checking function.The `paramflags` bit mask can be used to modify how a specific parameter is processed and the same mask is also used by the configuration module to return information about how the parameter has been processed. Bits definitions are described in the [`paramdef_t` structure description](./struct.md)


* `numparams` is the number of entries in the params array
* `prefix` is a character string to be appended to the parameters name, it defines the parameters position in the configuration file hierarchy (the section name in libconfig terminology).
* The returned value is the number of parameters which have been assigned a value or -1 if a severe error occured

```c
int config_libconfig_getlist(paramlist_def_t *ParamList, paramdef_t *params, int numparams, char *prefix)
```
* Reads multiple occurrences of a parameters array
* Calls the `config_<config source>_get` function for each list occurrence
* `params` points to an array of `paramdef_t` structures which describes the parameters in each occurrence of the list
* `ParamList`  points to a structure, where `paramarray` field points to an array of `paramdef_t` structure, allocated by the function. It is used to return the values of the parameters.
* The returned value is the number of occurrences in the list or -1 in case of severe error

## utility functions and macros

The configuration module also defines APIs to access the  `paramdef_t` and `configmodule_interface_t` fields. They are listed in the configuration module [include file `common/config/config_userapi.h`](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_userapi.h)

[Configuration module developer main page](../../config/devusage.md)
[Configuration module home](../../config.md)
