This tuto for 5G gNB design, with Open Cells main

# Top file: executables/ocp-gnb.c

the function main() initializes the data from configuration file

# The main thread is in ru_thread()
The infinite loop:
## rx_rf()
  Collect radio signal samples from RF board  
    called for each 5G NR slot  
    it blocks until data is available  
    the internal time comes from the RF board sampling numbers  
    (each sample has a incremental number representing a very accurate timing)  
raw incoming data is in buffer called "rxdata"
### nr_fep_full()
"front end processing" of uplink signal  
performs DFT on the signal  
same function (duplicates): phy_procedures_gNB_common_RX()
it computes the buffer rxdataF (for frequency) from rxdata (samples over time)
### gNB_top()
only compute frame numbre, slot number, ...
### ocp_rxtx()
main processing for both UL and DL  
all the context is in the passed structure UL_INFO  
the context is not very clear: there is a mutex on it,  
but not actual coherency (see below handle_nr_rach() assumes data is up-to-date)  

The first part (in NR_UL_indication, uses the data computed by the lower part (phy_procedures_gNB_uespec_RX), but for the previous slot  

Then, phy_procedures_gNB_uespec_RX will replace the data for the next run  

This is very tricky and not thread safe at all.

#### NR_UL_indication()  
This block processes data already decoded and stored in structures behind UL_INFO

* handle_nr_rach()  
process data from RACH primary detection  
if the input is a UE RACH detection
    * nr_schedule_msg2()
* handle_nr_uci()  
????	      
* handle_nr_ulsch()  
handles ulsch data prepared by nr_fill_indication()
* gNB_dlsch_ulsch_scheduler ()  
also calls "run_pdcp()", as this is not a autonomous thread, it needs to be called here to update traffic requests (DL) and to propagate waiting UL to upper layers  
* NR_Schedule_response()  
process as per the scheduler decided

#### L1_nr_prach_procedures()  
????
#### phy_procedures_gNB_uespec_RX()
* nr_decode_pucch0()  
actual CCH channel decoding  
* nr_rx_pusch()  
    * extracts data from rxdataF (frequency transformed received data)  
    * nr_pusch_channel_estimation()
    * nr_ulsch_extract_rbs_single()  
    * nr_ulsch_scale_channel()
    * nr_ulsch_channel_level()
    * nr_ulsch_channel_compensation()
    * nr_ulsch_compute_llr()  
this function creates the "likelyhood ratios"  
* nr_ulsch_procedures()
    * actual ULsch decoding
    * nr_ulsch_unscrambling()
    * nr_ulsch_decoding()
    * nr_fill_indication()   
populated the data for the next call to "NR_UL_indication()"

#### phy_procedures_gNB_TX()
* nr_common_signal_procedures()  
generate common signals
* nr_generate_dci_top()  
generate DCI: the scheduling informtion for each UE in both DL and UL
* nr_generate_pdsch()  
generate DL shared channel (user data)

### nr_feptx_prec()
tx precoding
### nr_feptx0
do the inverse DFT
### tx_rf()
send radio signal samples to the RF board  
the samples numbers are the future time for these samples emission on-air  