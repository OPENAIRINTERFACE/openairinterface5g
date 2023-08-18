The configuration module objectives are
1. Allow easy parameters management in oai, helping the development of a flexible, modularized and fully configurable softmodem.
2. Use a common configuration API in all oai modules
3. Allow development of alternative configuration sources without modifying the oai code. Today the only delivered configuration source is the libconfig format configuration file.

As a developer you may need to look at these sections:

* [add parameters in an existing section](devusage/addaparam.md)
* [add a parameter set, in a new section](devusage/addparamset.md)
* [configuration module API](devusage/api.md)
* [configuration module public structures](devusage/struct.md)

Whatever your need is, configuration module usage examples can be found in oai sources:
*  complex example, using all the configuration module functionalities, including parameter checking:
[NB-IoT configuration code](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair2/ENB_APP/NB_IoT_config.c) and [NB-IoT configuration include file](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair2/ENB_APP/NB_IoT_paramdef.h)
*  very simple example, just reading a parameter set corresponding to a dedicated section:  the telnetsrv_autoinit function in [common/utils/telnetsrv/telnetsrv.c, around line 726](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/utils/telnetsrv/telnetsrv.c#L726)
*  an example with run-time definition of parameters, in the logging sub-system: the log_getconfig function at the top of [openair2/UTIL/LOG/log.c](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/openair2/UTIL/LOG/log.c)


[Configuration module home](../config.md)
