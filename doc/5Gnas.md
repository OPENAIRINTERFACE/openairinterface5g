<style type="text/css" rel="stylesheet">

body {
   font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
   font-size: 13px;
   line-height: 18px;
   color: #fff;
   background-color: #110F14;
}
  h2 { margin-left: 20px; }
  h3 { margin-left: 40px; }
  h4 { margin-left: 60px; }

.func2 { margin-left: 20px; }
.func3 { margin-left: 40px; }
.func4 { margin-left: 60px; }

</style>


This tuto for 5G gNB NAS design
{: .text-center}

# source files

executables/ocp-gnb.c: a main loop to create a debuggable gNB, see document SW_archi.md
openair2/RRC/NR/nr_ngap_gNB.c: skeleton for interface with NGAP
openair3/NAS/COMMON/milenage.h: a simple milenage implementation, depend only on crypto library
openair3/NAS/COMMON/NR_NAS_defs.h: messages defined for NAS implemented in C: C struct, C enums and automatic conversion to labels for debug messages
openair3/NAS/COMMON/nr_common.c: 5G NAS common code between gNB and UE
openair3/UICC/usim_interface.c: UICC simulation for USIM messages

openair3/NAS/NR_UE/ue_process_nas.c: NAS code for UE

openair3/NAS/gNB/network_process_nas.c: NAS code for gNB running without 5GC

openair3/TEST/test5Gnas.c: unitary testing of NAS messages: encodes+decode a standard network entry, mixing code from UE and 5GC (or gNB in noCore)

# USIM simulation
A new USIM simulation, that parameters are in regular OAI config files

## To open the USIM
init_uicc() takes a parameter: the name of the block in the config file
In case we run several UEs in one process, the variable section name allows to make several UEs configurations in one config file.

The NAS implementation uses this possibility.

# UE side
UEprocessNAS() is the entry for all messages.
It decodes the message type and call the specific function for each messgae

## Identityrequest + IdentityResponse
When the UE receives the request, it stores the 5GC request parameters in the UE context ( UE->uicc pointer)
It calls "scheduleNAS", that would trigger a answer (seeing general archtecture, it will probly be a itti message to future RRC or NGAP thread).  

When the scheduler wants to encode the answer, it calls identityResponse()
The UE search for a section in the config file called "uicc"
it encodes the NAS answer with the IMSI, as a 4G compatible authentication.
A future release would encode 5G SUPI as native 5G UICC.

## Authenticationrequest + authenticationResponse
When the UE receives the request, it stores the 5GC request parameters in the UE context ( UE->uicc pointer)
It calls "scheduleNAS", that would trigger a answer (seeing general archtecture, it will probly be a itti message to future RRC or NGAP thread).  

When the scheduler wants to encode the answer, it calls authenticationResponse()
The UE search for a section in the config file called "uicc"
It uses the Milenage parameters of this section to encode a milenage response
A future release would encode 5G new authentication cyphering functions.

## SecurityModeCommand + securityModeComplete
When the UE receives the request it will:
Selected NAS security algorithms: store it for future encoding
Replayed UE security capabilities: check if it is equal to UE capabilities it sent to the 5GC
IMEISV request: to implement
ngKSI: Key set Indicator, similator to 4G to select a ciphering keys set

When the scheduler wants to encode the answer, it calls registrationComplete() 

## registrationComplete
To be defined in UE, this NAS message is done after RRC sequence completes

# gNB side
gNB NGAP thread receives the NAS message from PHY layers
In normal mode, it send it to the core network with no decoding.

Here after, the gNB mode "noCore" processing in gNB: NGAP calls the entry function: processNAS() instead of forwarding the packet to the 5GC

## RRCModeComplete + Identityrequest
When the gNB completes RRC attach, it sends a first message to NGAP thread.
Normal processing sends NGAP initial UE message, in noCore mode, it should call: identityRequest() that encode the NAS identity request inside the gNB.  
The gNB NGAP thread then should call the piece of code that forward the message to the UE as when it receives a NAS message from 5GC.  

## All NAS coming from UE
When NGAP thread receives a NAS message from lower layers, it encapsulates it to forward it to the 5GC. In "noCore" mode it calls processNAS(). 

This function stores in the gNB the NAS data, then it should encode the next message (identity, authentication, security mode) or do nothing (registration complete).


<div class="panel panel-info">
**Note**
{: .panel-heading}
<div class="panel-body">


</div>
</div>

