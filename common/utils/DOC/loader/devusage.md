The configuration module objectives are 
1. Allow easy parameters management in oai, helping the development of a flexible, modularized and fully configurable softmodem.
1. Use a common configuration API in all oai modules
1. Allow development of alternative configuration sources without modifying the oai code. Today the only delivered configuration source is the libconfig format configuration file.  

As a developer you may need to look at these sections:

* [loading a shared library](devusage/loading.md)
* [loader API](devusage/api.md) 
* [loader public structures](devusage/struct.md)  

Loader usage examples can be found in oai sources:  

*  device and transport initialization code: [function `load_lib` in *targets/ARCH/COMMON/__common_lib.c__* ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/targets/ARCH/COMMON/common_lib.c#L91) 
*  turbo encoder and decoder initialization: [function `load_codinglib`in *openair1/PHY/CODING/__coding_load.c__*](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair1/PHY/CODING/coding_load.c#L113)

[loader home page](../loader.md)
