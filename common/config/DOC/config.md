# OAI configuration module

The configuration module provides an api that other oai components can use to get parameters at init time. It defines a parameter structure, used to describe parameters attributes (for example name and  type). The same structure includes a pointer to the variable where the configuration module writes the parameter value, at run time, when calling config_get or config_getlist functions. For each parameter a function to check the value can be specified and pre-defined functions  are provided for some common parameter validations (integer in a range or in a list, string in a list).  The module also include an api to check that no unknown options have been entered on the command line and a a mechanism to display a help text for suppoted parameters.

## Documentation

* [runtime usage](config/rtusage.md)
* [developer usage](config/devusage.md)
* [module architecture](config/arch.md)

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
