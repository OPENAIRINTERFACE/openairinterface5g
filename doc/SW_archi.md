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


This tuto for 5G gNB design, with Open Cells main
{: .text-center}

# Top file: executables/ocp-gnb.c

the function main() initializes the data from configuration file

# The main thread is in ru_thread()
The infinite loop:
## rx_rf()
  Collect radio signal samples from RF board  
    all SDR processing is triggered by I/Q sample reception and it's date (timestamp)  
    TX I/Q samples will have a date in the future, compared to RX timestamp  
    called for each 5G NR slot  
    it blocks until data is available  
    the internal time comes from the RF board sampling numbers  
    (each sample has a incremental number representing a very accurate timing)  
raw incoming data is in buffer called "rxdata"  
    We derivate frame number, slot number, ... from the RX timestamp
{: .func2}
## nr_fep_full()
"front end processing" of uplink signal  
performs DFT on the signal  
same function (duplicates): phy_procedures_gNB_common_RX()  
it computes the buffer rxdataF (for frequency) from rxdata (samples over time)  
rxdataF is the rxdata in frequency domain, phase aligned
{: .func3}
## gNB_top()
only compute frame numbre, slot number, ...
{: .func3}
## ocp_rxtx()
main processing for both UL and DL  
start by calling oai_subframe_ind() that trigger processing in pnf_p7_subframe_ind() purpose ???  
all the context is in the passed structure UL_INFO  
the context is not very clear: there is a mutex on it,  
but not actual coherency (see below handle_nr_rach() assumes data is up-to-date)  
The first part (in NR_UL_indication, uses the data computed by the lower part (phy_procedures_gNB_uespec_RX), but for the **previous** slot  
Then, phy_procedures_gNB_uespec_RX will hereafter replace the data for the next run  
This is very tricky and not thread safe at all.
{: .func3}

### NR_UL_indication()  
This block processes data already decoded and stored in structures behind UL_INFO
{: .func4}

* handle_nr_rach()  
process data from RACH primary detection  
if the input is a UE RACH detection
{: .func4}
    * nr_schedule_msg2()
{: .func4}
* handle_nr_uci()  
handles uplink control information, i.e., for the moment HARQ feedback.
{: .func4}
* handle_nr_ulsch()  
handles ulsch data prepared by nr_fill_indication()
{: .func4}
* gNB_dlsch_ulsch_scheduler ()  
the **scheduler** is called here, see dedicated chapter
{: .func4}
* NR_Schedule_response()  
process as per the scheduler decided
{: .func4}

### L1_nr_prach_procedures()  
????
{: .func4}
### phy_procedures_gNB_uespec_RX()
* nr_decode_pucch0()  
actual CCH channel decoding form rxdataF (rx data in frequency domain)  
populates UL_INFO.uci_ind, actual uci data is in gNB->pucch  
{: .func4}
* nr_rx_pusch()  
{: .func4}
    * extracts data from rxdataF (frequency transformed received data)
{: .func4}
    * nr_pusch_channel_estimation()
{: .func4}
    * nr_ulsch_extract_rbs_single()
{: .func4}
    * nr_ulsch_scale_channel()
{: .func4}
    * nr_ulsch_channel_level()
{: .func4}
    * nr_ulsch_channel_compensation()
{: .func4}
    * nr_ulsch_compute_llr()  
this function creates the "likelyhood ratios"  
{: .func4}
* nr_ulsch_procedures()
{: .func4}
    * actual ULsch decoding
{: .func4}
    * nr_ulsch_unscrambling()
 {: .func4}
   * nr_ulsch_decoding()
 {: .func4}
   * nr_fill_indication()   
populate the data for the next call to "NR_UL_indication()"  
it would be better to call **NR_UL_indication()** now instead of before (on previous slot)
{: .func4}

### phy_procedures_gNB_TX()
* nr_common_signal_procedures()  
generate common signals
{: .func4}
* nr_generate_dci_top()
generate DCI: the scheduling informtion for each UE in both DL and UL
{: .func4}
* nr_generate_pdsch()  
generate DL shared channel (user data)
{: .func4}

### nr_feptx_prec()
tx precoding
{: .func3}
### nr_feptx0
do the inverse DFT
{: .func3}
### tx_rf()
send radio signal samples to the RF board  
the samples numbers are the future time for these samples emission on-air
{: .func3}

# Scheduler

The main scheduler function  is called by the chain: nr_ul_indication()=>gNB_dlsch_ulsch_scheduler()
It calls sub functions to process each physical channel (rach, ...)  
The scheduler uses and internal map of used RB: vrb_map and vrb_map_UL, so each specific channel scheduler can see the already filled RB in each subframe (the function gNB_dlsch_ulsch_scheduler() clears these two arrays when it starts)   

The scheduler also calls "run_pdcp()", as this is not a autonomous thread, it needs to be called here to update traffic requests (DL) and to propagate waiting UL to upper layers  
After calling run_pdcp, it updates "rlc" time data but it doesn't actually process rlc 
it sends a iiti message to activate the thread for RRC, the answer will be asynchronous in ????  

Calls schedule_nr_mib() that calls mac_rrc_nr_data_req() to fill MIB,  

Calls schedule_nr_prach() which schedules the (fixed) PRACH region one frame in
advance.

Calls nr_csi_meas_reporting() to check when to schedule CSI in PUCCH.

Calls nr_schedule_RA(): checks RA process 0's state. Schedules Msg.2 via
nr_generate_Msg2() if an RA process is ongoing, and pre-allocates the Msg. 3
for PUSCH as well.

Calls nr_schedule_ulsch(): It is divided into the "preprocessor" and the
"postprocessor": the first makes the scheduling decisions, the second fills
nFAPI structures to indicate to the PHY what it is supposed to do. To signal
which users have how many resources, the preprocessor populates the
NR_sched_pusch_t (for values changing every TTI, e.g., frequency domain
allocation) and NR_sched_pusch_save_t (for values changing less frequently, at
least in FR1 [to my understanding], e.g., DMRS fields when the time domain
allocation stays between TTIs) structures. Furthermore, the preprocessor is an
exchangeable module that schedules differently based on a particular
use-case/deployment type, e.g., one user for phytest [in
nr_ul_preprocessor_phytest()], multiple users in FR1
[nr_fr1_ulsch_preprocessor()], or maybe FR2 [does not exist yet]:
* calls preprocessor via pre_processor_ul(): the preprocessor is responsible
  for allocating CCEs (using allocate_nr_CCEs()) and deciding on resource
  allocation for the UEs including TB size. Note that we do not yet have
  scheduling requests. What it typically does:
  1)  check whether the current frame/slot plus K2 is an UL slot, and return if
      not.
  2)  Find first free start RB in vrb_map_UL, and as many free consecutive RBs
      as possible.
  3)  Either set up resource allocation directly (e.g., for a single UE,
      phytest), or call into a function to perform actual resource allocation.
      Currently, this is done using pf_ul() which implements a basic
      proportional fair scheduler:
      * for every UE, check for retransmission and allocate as necessary
      * Calculate DMRS stuff (nr_set_pusch_semi_static())
      * Calculate the PF coefficient and put eligible UEs into a list
      * Allocate resources to the UE(s) with the highest coefficient
  4)  Mark used resources in vrb_map_UL.
* loop through all users: get a HARQ process as indicated through the
  preprocessor, update statistics, fill nFAPI structures directly for PUSCH,
  and call config_uldci() and fill_dci_pdu_rel15() for DCI filling and PDCCH
  messages.

Calls nr_schedule_ue_spec(). It is divided into the "preprocessor" and the
"postprocessor": the first makes the scheduling decisions, the second fills
nFAPI structures to indicate to the PHY what it is supposed to do. To signal
which users have how many resources, the preprocessor populates the
NR_UE_sched_ctrl_t structure of affected users. In particular, the field rbSize
decides whether a user is to be allocated. Furthermore, the preprocessor is an
exchangeable module that schedules differently based on a particular
use-case/deployment type, e.g., one user for phytest [in
nr_preprocessor_phytest()], multiple users in FR1
[nr_fr1_dlsch_preprocessor()], or maybe FR2 [does not exist yet].
* calls preprocessor via pre_processor_dl(): the preprocessor is responsible
  for allocating CCEs and PUCCH (using allocate_nr_CCEs() and
  nr_acknack_scheduling()) and deciding on the frequency/time domain
  allocation including the TB size. What it typically does:
  1)  Check available resources in the vrb_map
  2)  Checks the quantity of waiting data in RLC
  3)  Either set up resource allocation directly (e.g., for a single UE,
      phytest), or call into a function to perform actual resource allocation.
      Currently, this is done using pf_dl() which implements a basic
      proportional fair scheduler:
      * for every UE, check for retransmission and allocate as necessary
      * Calculate the PF coefficient and put eligible UEs into a list
      * Allocate resources to the UE(s) with the highest coefficient
  4)  Mark taken resources in the vrb_map
* loop through all users: check if a new TA is necessary. Then, if a user has
  allocated resources, update statistics (round, sent bytes), update HARQ
  process information, and fill nFAPI structures (allocate a DCI and PDCCH
  messages, TX_req, ...)

# RRC
RRC is a regular thread with itti loop on queue: TASK_RRC_GNB
it receives it's configuration in message NRRRC_CONFIGURATION_REQ, then real time mesages for all events: S1/NGAP events, X2AP messages and RRC_SUBFRAME_PROCESS  
  
RRC_SUBFRAME_PROCESS message is send each subframe  
  
how does it communicate to  scheduler ?  


# RLC
RLC code is new implementation, not using OAI mechanisms: it is implemented directly on pthreads, ignoring OAI common functions.  
It is a library, running in thread RRC but also in PHY layer threads and some bits in pdcp running thread or F1 interface threads.

RLC data is isolated and encapsulated.
It is stored under a global var: nr_rlc_ue_manager
The init function rlc_module_init() populates this global variable.
A small effort could lead us to return the pointer to the caller of rlc_module_init() (internal type: nr_rlc_ue_manager_internal_t)  
but it returns void.  
It could return the initialized pointer (as FILE* fopen() for example), then the RLC layer could have multiple instances in one process.
Even, a future evolution could remove this global rlc layer: rlc can be only a library that we create a instance for each UE because it doesn't shareany data between UEs.

For DL (respectively from UL in UE), the scheduler need to know the quantity of data waitin to be sent: it calls mac_rlc_status_ind()
That "peek" the size of the waiting data for a UE.
The scheduler then push orders to lower layers. The transport layer will actually pull data from RLC with: mac_rlc_data_req()  
the low layer push data into rlc by: mac_rlc_data_ind()  
Still on DL (gNB side), PDCP push incoming data into RLC by calling: rlc_data_req()

For UL, the low layer push data into rlc by: mac_rlc_data_ind()  
Then, rlc push it to pdcp by calling pdcp_data_ind() from a complex rlc internal call back (deliver_sdu())  

When adding a UE, external code have to call nr_rrc_rlc_config_asn1_req(), to remove it: rrc_rlc_remove_ue()  
Inside UE, channels called drd or srb can be created: ??? and deleted: rrc_rlc_config_req()

nr_rlc_tick() must be called periodically to manage the internal timers 

successful_delivery() and max_retx_reached(): in ??? trigger, the RLC sends a itti message to RRC: RLC_SDU_INDICATION (neutralized by #if 0 right now)

#PDCP

The PDCP implementation is also protected through a general mutex.  
The design is very similar to rlc layer. The pdcp data is isolated and encapsulated.

pdcp_layer_init(): same as rlc init  
we have to call a second init function: pdcp_module_init() 

At Tx side (DL in gNB), pdcp_data_req() is the entry function that the upper layer calls.  
The upper layer can be GTP or a PDCP internal thread enb_tun_read_thread() that read directly from Linux socket in case we skip 3GPP core implementation.
PDCP internals for  pdcp_data_req() is thread safe: inside pdcp_data_req_drb(), the pdcp manager protects with the mutex the access to the SDU receiving function of PDCP (recv_sdu() callback, corresponding to nr_pdcp_entity_drb_am_recv_sdu() for DRBs). When it needs, the pdcp layer push this data to rlc by calling : rlc_data_req()  

Also, incoming downlink sdu can comme from internal RRC: in this case, pdcp_run() reads a itti queue, for message RRC_DCCH_DATA_REQ, to0 only call 'pdcp_data_req()'

At Rx side, pdcp_data_ind() is the entry point that receives the data from RLC.
- Inside pdcp_data_ind(), the pdcp manager mutex protects the access to the PDU receiving function of PDCP (recv_pdu() callback corresponding to nr_pdcp_entity_drb_am_recv_pdu() for DRBs)
- Then deliver_sdu_drb() function sends the received data to GTP thread through an ITTI message (GTPV1U_ENB_TUNNEL_DATA_REQ).

pdcp_config_set_security(): not yet developped

nr_DRB_preconfiguration(): the mac layer calls this for ???

nr_rrc_pdcp_config_asn1_req() adds a UE in pdcp, pdcp_remove_UE() removes it

# GTP
Gtp + UDP are two twin threads performing the data plane interface to the core network
The design is hybrid: thread and inside other threads calls. It should at least be protected by a mutex.
## GTP thread
Gtp thread has a itti interface: queue TASK_GTPV1_U  
The interface is about full definition: control messages (create/delet GTP tunnels) and data messages (user plane UL and DL).  
PDCP layer push to the GTP queue (outside UDP thread that do almost nothing and work only with GTP thread) is to push a UL packet.


## GTP thread running code from other layers
gtp thread calls directly pdcp_data_req(), so it runs inside it's context internal pdcp structures updates

## inside other threads
gtpv1u_create_s1u_tunnel(), delete tunnel, ... functions are called inside the other threads, without mutex.

# New GTP
## initialization
Coexistance until full merge with legacy GTP
cmake new option: NEW_GTPU to use the new implementation (it changes for the entire executable)
It is possible to use both old and new GTP in same executable because the itti task and all functions names are different 
Current status of new implementation: not tested, X2 not developped, 5G new GTP option not developped, remain issues on data coming from void: muid, enb_flag, ...

ocp_gtpv1uTask(): this creates only the thread, doesn't configure anything
gtpv1Init(): creates a listening socket to Linux for a given reception and select a local IP address

newGtpuCreateTunnel() this function will replace the xxx_create_tunnel_xxx() for various cases
This creates a outgoing context for a teid (in input), it computes and return the incoming teid that will be used for incoming packets
These teids and in a "instance", so in a Linux socket: same teid can co-exist for different sockets
 Remain here a lack to fill: the information given in the legacy funtions is not enough to fullfil the data needed by the callback
stuff like enb_flag, but also mui and more important data are not given explicitly by any legacy function (gtpv1u_create_s1u_tunnel), but the legacy and the new interface to lower layer (like pdcp) require this data.
The datamodel is still not fully understood, so this data source remain unknown
A new parameter is the callback function: will be pdpcp_data_req() and gtpv_data_req() (x2 case) for existing implementation and later other call backs like the F1-U implementation.

incoming packets
the gtp layer retrieves the data, the teid, find out the related data: rnti, bearer and quite a lot of other parameters (not clear why, because it looks like all is statefull, so the lower layer should have the context) 
if lower layers can be stateless, it is a good idea to keep the context in the gtp layer and pass it to the callback, but the design remain obfuscated.

# NGAP
NGAP would be a itti thread as is S1AP (+twin thread SCTP that is almost void processing)?  
About all messages are exchanged with RRC thread  


<div class="panel panel-info">
**Note**
{: .panel-heading}
<div class="panel-body">


</div>
</div>

