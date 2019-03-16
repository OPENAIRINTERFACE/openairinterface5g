# Running OAI softmodem

After you have [built the softmodem executables](BUILD.md) you can go to the build directory `cmake_targets/lte_build_oai/build/` and start testing some use cases. 

## rf simulator

 The rf simulator is a oai device replacing the radio heads (for example the USRP device). It allows connecting the oai UE and the oai eNodeB through a network interface carrying the time-domain samples, getting rid of over the air unpredictable perturbations. This is the ideal tool to check signal processing algorithms implementation.

It is planned to enhance this simulator with the following functionalities:

- Support for multiple UE connections
- Support for multiple eNodeB for hand-over tests
- Spport for channel modeling

   This is an easy use-case to setup and test, as no specific hardware is required. The [rfsimulator page](../targets/ARCH/rfsimulator/README.md ) contains the detailed documentation.

## l2 nfapi simulator

  As for the rf simulator, no specific hardware is required. The [L2 nfapi simlator page](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/l2-nfapi-simulator/l2-nfapi-simulator-w-S1-same-machine) contains the detailed documentation.

## Monolithic eNodeB with USRP radio head

1. ##  

