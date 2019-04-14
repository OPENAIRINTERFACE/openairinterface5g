The loader objectives are
1. provides a common mechanism to prevent an executable to include code that is only used under specific configurations, without rebuilding the code
1. Provide a common mechanism to allow to choose between different implementations of a given set of functions, without rebuilding the code

As a developer you may need to look at these sections:

* [loading a shared library](devusage/loading.md)
* [loader API](devusage/api.md)
* [loader public structures](devusage/struct.md)

Loader usage examples can be found in oai sources:

*  device and transport initialization code: [function `load_lib` in *targets/ARCH/COMMON/__common_lib.c__* ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/targets/ARCH/COMMON/common_lib.c#L91)
*  turbo encoder and decoder initialization: [function `load_codinglib`in *openair1/PHY/CODING/__coding_load.c__*](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair1/PHY/CODING/coding_load.c#L113)

[loader home page](../loader.md)
