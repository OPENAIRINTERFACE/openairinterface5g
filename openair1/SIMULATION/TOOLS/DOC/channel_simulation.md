# OAI channel simulation feature

oai includes a channel simulation feature that any component can use to alter time domain samples of a RF channel by applying pre-defined models as defined, for example, in 3GPP TR 36.873 or TR 38.901

Definition, configuration and run-time modification of a channel model are implemented in common code included in UEs, gNb, eNB and  used when running with  the rfsimulator or the L1 simulator. Phy simulators are also using channel simulation but configuration is done via dedicated command line options. The rfsimulator is the only option to get access to all the configurations and run-time modifications features of oai channel simulation.

## Documentation

* [runtime usage](rtusage.md)
* [developer usage](devusage.md)
* [module architecture](arch.md)

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
