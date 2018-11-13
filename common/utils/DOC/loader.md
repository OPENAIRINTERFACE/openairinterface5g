# OAI shared library loader

Shared libraries usage is modularization mechanism which provides the following advantages:
1. Prevents including in the main executable code which is not used in a given configuration
1. Provides flexibility, several implementation of a given functionality can be chosen at run-time, without any compilation step. For example you can build several devices (USRP, BladeFR, LimeSDR)  and choose which one you want to use on the command line or via the configuration.
1.  Makes code evolution easier: as soon as the shared library interface is clearly defined, you can work on the functionality implemented in a shared library while regularly updating the other components of the code. You can decide to develop your own version of a functionality, decide to deliver it or not,  letting the user decide wwhat version he wants to use.

The main drawback is a performance cost at init time, when loading libraries.


## Documentation

* [runtime usage](loader/rtusage.md)
* [developer usage](loader/devusage.md)
* [module architecture](loader/arch.md)

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)