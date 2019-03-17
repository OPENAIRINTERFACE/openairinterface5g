# Running OAI softmodem

After you have [built the softmodem executables](BUILD.md) you can set your default directory  to the build directory `cmake_targets/lte_build_oai/build/` and start testing some use cases. Below, the description of the different oai functionalities should help you choose the oai configuration that suits your need. 

## rf simulator

 The rf simulator is a oai device replacing the radio heads (for example the USRP device). It allows connecting the oai UE and the oai eNodeB through a network interface carrying the time-domain samples, getting rid of over the air unpredictable perturbations. This is the ideal tool to check signal processing algorithms and protocols implementation.

It is planned to enhance this simulator with the following functionalities:

- Support for multiple UE connections,each UE being a `lte-uesoftmodem` instance.
- Support for multiple eNodeB for hand-over tests
- Support for channel modeling

   This is an easy use-case to setup and test, as no specific hardware is required. The [rfsimulator page](../targets/ARCH/rfsimulator/README.md ) contains the detailed documentation.

## l2 nfapi simulator

This simulator connects a eNodeB and UEs through a nfapi interface, short-cutting the L1 layer. The objective of this simulator is to allow multi UEs simulation, with a large number of UEs (ideally up to 255 ) .Here to ease the platform setup, UEs are simulated via a single `lte-uesoftmodem` instance. Today the CI tests just with one UE and architecture has to be reviewed to allow a number of UE above about 16. This work is on-going.

  As for the rf simulator, no specific hardware is required. The [L2 nfapi simlator page](L2NFAPI.md) contains the detailed documentation.

## l1 simulator

The l1 simulator is using the ethernet fronthaul protocol, as used to connect a RRU and a RAU to connect UEs and a eNodeB. UEs are simulated in a single `lte-uesoftmodem` process, as for the nfapi simulator. 

The [L1 simulator page](L1SIM.md) contains the detailed documentation.

## noS1 mode

The noS1 mode is now available via the `--noS1`command line option. It can be used with simulators, described above, or when using oai with true RF boards. Only the oai UE can be connected to the oai eNodeB in noS1 mode.

By default the noS1 mode is using linux tun interfaces to send or receive ip packets to/from the linux ip stack. using the `--nokrnmod 0`option you can enforce kernel modules instead of tun.

noS1 code has been revisited, it has been tested with the rf simulator, and tun interfaces. More tests are on going and CI will soon include noS1 tests.

## Using USRP radio head

[Monolithic eNodeB](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/HowToConnectCOTSUEwithOAIeNBNew)







[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

