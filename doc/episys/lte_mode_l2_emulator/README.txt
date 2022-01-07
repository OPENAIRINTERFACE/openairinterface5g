There are fundamental changes to the L2 emulation mechanism; these changes allow the
user to run multiple UEs in separate Linux processes/machines/VMs/etc. They use a separate
entity between the UE(s) and eNB. The UEs use nFAPI to communicate with the eNB. The
nFAPI interface allows us to run in an emulated layer 2 mode, meaning that we are bypassing
the layer 1 (PHY) functionality. Because we are bypassing the PHY layer,
channel modeling capabilties have been added in the LTE UE phy_stub_ue.c file. To understand
the interfaces between the different components associated with LTE mode, the image
functional_diagram_proxy_lte.png has been provided.

This functionality allows the user to plug in their own channel model and emulate the packet dropping procedure
in real time. The channel modeling has not been provided by EpiSci, but the OAI code
base already has some BLER curves available for use. The channel modeling functionality
that is included in the phy_stub_ue.c file only includes the downlink channel modeling.
Any uplink channel modeling must be conducted in some sort of proxy, which would sit
between the UEs and eNB. (A description of the downlink channel modeling is shown in the
Channel_Abstraction_UE_Handling_LTE.PNG image).

The updates to the OAI code base removed some latent bugs, added multi-UE scalability,
and were tested with a standard bypass proxy between the UE(s) and eNB. The bypass proxy is
publicly available on GitHub (https://github.com/EpiSci/oai-lte-multi-ue-proxy). With this package,
various multi-UE scenarios can be tested without the overhead of PHY-layer features
of underlying radios. The features added in the L2 Emulator Mode for LTE are:

 - Ease of use of gprof and address sanitizer for debugging purposes
 - Updated json files to allow for GDB, real-time debugging capabilities
 - Updated logging features to minimally log only key connection milestones. This improves scalability of multiple UEs.
The logging mechanism described here is located in the log.c and log.h files. The LOG_MINIMAL
function allows us to remove most OAI logging and includes LOG_A(...) logs and above. The LOG_A
logs were chosen as analysis logs to meet EpiSci's internal testing procedure. The LOG_As
only include logs that are considered to be milestones in a given test. For example, a
log indicating that the RACH procedure has been completed for LTE. To select full logging, 
set LOG_MINIMAL = 0 in the log.h file.
 - Updated logging to include time stamp for timing analysis
 - Updated memory allocation procedures to correct size requirements
 - Added debugging features to handle signal terminations
 - nfapi.c pullarray8 fix invalid pointer math
 - Overlapping destination and source memory in memcpy, so updated to memmove to check for this bug
 - Advanced error checking mechanisms in critical pack and unpack functions
 - Created option for CPU assignment to UE to improve scalability
 - Added EPC integration to allow multiple individual UE entities to each have their USIM information parsed by the executables
 - Updated random value seeds to minimize probability of error in generation of random values
 - Enables capability round robin scheduler if desired
 - Enables capability real time scheduler if desired
 - Added new standalone functions to the UE phy-layer (phy_stub_ue.c) to incorporate individual UE entities
 - Updated sending and packing functions in UE (lte_ue.c) to incorporate new standalone changes
 - Incorporated semaphores to control timing of incoming downlink packets
 - Implemented new queuing system to handle message exchange from UE to eNB and vice versa
 - Updated global value in nFAPI for size of subframe
 - Updated global value to increase scalability in system
