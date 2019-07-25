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
void End_configmodule(void)
```
* Free memory which has been allocated by the configuration module since its initialization.
* Possibly calls the `config_<config source>_end` function

```c
int config_get(paramdef_t *params,int numparams, char *prefix)
```
* Reads as many parameters as described in params, they must all be in the same configuration file section
* Calls the `config_<config source>_get` function
* Calls the `config_process_cmdline` function
* `params` points to an array of `paramdef_t` structures which describes the parameters to be read, possibly including a pointer to a checking function. The following bits can possibly be set in the `paramflags` mask before calling
 - `PARAMFLAG_MANDATORY`: -1 is returned if the parameter is not explicitly defined in the config source.
 - `PARAMFLAG_DISABLECMDLINE`: parameter cannot be modified via the command line
 - `PARAMFLAG_DONOTREAD`: ignore the parameter, can be used at run-time, to alter a pre-defined `paramdef_t` array which is used in several `config_get` or/and `config_getlist` calls.
 - `PARAMFLAG_NOFREE`: do not free the memory possibly allocated by the config module to store the value of the parameter. Default behavior is for the config module to free the memory it has allocated when the `config_end` function is called.
 - `PARAMFLAG_BOOL`: Only relevant for integer types. tell the config module that when processing the command line, the corresponding option can be specified without any arggument and that in this case it must set the value to 1.

* `params` is also used as an output parameter, `< XXX >ptr >` field  is used by the config module to store the value it has read. The following bits can possibly be set in the `paramflags` mask after the call:
  - `PARAMFLAG_MALLOCINCONFIG`: memory has been allocated for the ` < XXX >ptr > ` field
  - `PARAMFLAG_PARAMSET`: parameter has been found in the config source, it is not set to default value.
  - `PARAMFLAG_PARAMSET`: parameter has been set to its default value
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


[Configuration module developer main page](../../config/devusage.md)
[Configuration module home](../../config.md)
