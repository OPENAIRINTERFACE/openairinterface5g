There are fundamental changes to the L2 emulation mechanism; these changes allow the
user to run multiple UEs in separate Linux processes/machines/VMs/etc. They use a separate
entity between the UE(s) and eNB/gNB. The UEs use nFAPI to communicate with the eNB/gNB. The
nFAPI interface allows us to run in an emulated L2 mode, meaning that we are bypassing
the layer 1 (PHY) layer functionality. Becasue we are bypassing the PHY layer, special
channel modeling capabilty has been added in the LTE UE phy_stub_ue.c file. To understand
the interfaces between the different components associated with NSA mode, the image
functional_diagram_proxy_nsa.png has been provided.

This functionality allows the user to plug in their own channel model and emulate the packet dropping procedure
in real time. The channel modeling has not been provided by EpiSci, but the OAI code
base already has some BLER curves available for use. The chanel modeling functionality
that is included in the phy_stub_ue.c file only includes the downlink channel modeling.
Any uplink channel modeling must be conducted in some sort of proxy, which would sit
between the UEs and eNB/gNB. (A description of the downlink channel modeling is shown in the
Channel_Abstraction_UE_Handling_LTE.PNG image).

The updates to the OAI code base removed some latent bugs, added multi-UE scalability,
and were tested with a standard bypass proxy between the UE(s) and eNB/gNB. The bypass proxy is
publicly available on GitHub (https://github.com/EpiSci/oai-lte-multi-ue-proxy). With this package,
various multi-UE scenarios can be tested without the overhead of PHY-layer features
of underlying radios. The added features to the OAI code base are listed below.

 - Ease of use of gprof and address sanitizer for debugging purposes
 - Updated json files to allow for GDB, real-time debugging capabilities
 - Updated logging features to minimally log only key connection milestones. This improves scalability of multiple UEs.
The logging mechanism described here is located in the log.c and log.h files. The LOG_MINIMAL
function allows us to remove most logs and include LOG_A(...) logs and above. The LOG_A
logs were chosen as analysis logs to meet EpiSci's internal testing procedure. The LOG_As
only include logs that are considered to be milestones in a given test. For example, a
log indicating that the CFRA procedure has been completed for NSA mode. To revert to full logging, 
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


Additionally, NSA mode includes the establishment between an NR UE and the gNB via the LTE UE and eNB
connection. For NSA mode, the downlink channel abstraction has not been added to the feature set yet.
NSA mode has been tested and is fully functional with EpiSci's public version of the nFAPI proxy
located at https://github.com/EpiSci/oai-lte-multi-ue-proxy
NSA mode establishment includes the following steps:

 - First UE capability enquiry is sent to NR UE
 - NR UE updates and fills first capability enquiry
 - eNB request for a measurement report
 - Measurement request is sent to NR UE
 - Measurement report is filled in NR UE
 - Measurement report is received in eNB
 - Request for second UE capability enquiry
 - Send second enquiry to NR UE
 - Second capability information is filled by NR UE
 - Second capability information is sent to eNB
 - NR UE capability information is received in gNB
 - Periodic measurement report received in gNB
 - RRCReconfigurationRequest is sent to NR UE
 - RRCReconfigurationComplete is sent from NR UE
 - RACH is received in the gNB
 - RAR is received in NR UE
 - Msg 3 is generated and sent to gNB
 - CFRA procedure is complete

Please note, the current status is:
 - A single NSA UE is able to ping an external source
 - Up to 4 NSA UEs can complete the CFRA procedure with a single gNB
 - UE-to-UE traffic is still in work (incomplete due to gNB crashing)
 - The gNB is crashing regularly and is still in work

Test Setup:
For launching the multi-UE for NSA mode, there is a provided test script in the
public EpiSci GitHub multi-ue-proxy repository. However, a brief outline of the
test set up is included below.

    If running multiple NSA UEs in a single machine, you will need to launch
    multiple LTE and NR UE processes. For example, to launch a 2 NSA UE scenario
    there will be a total of 7 processes running for this particular scenario.
    These processes are LTE UE #1, LTE UE #2, NR UE #1, NR UE #2, eNB, gNB and the
    proxy. A detailed description of the launch processes can be found at
    https://github.com/EpiSci/oai-lte-multi-ue-proxy/blob/master/README.md

