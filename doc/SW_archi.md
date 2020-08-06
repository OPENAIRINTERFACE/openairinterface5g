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
    called for each 5G NR slot  
    it blocks until data is available  
    the internal time comes from the RF board sampling numbers  
    (each sample has a incremental number representing a very accurate timing)  
raw incoming data is in buffer called "rxdata"
{: .func2}
### nr_fep_full()
"front end processing" of uplink signal  
performs DFT on the signal  
same function (duplicates): phy_procedures_gNB_common_RX()
it computes the buffer rxdataF (for frequency) from rxdata (samples over time)
{: .func3}
### gNB_top()
only compute frame numbre, slot number, ...
{: .func3}
### ocp_rxtx()
main processing for both UL and DL  
all the context is in the passed structure UL_INFO  
the context is not very clear: there is a mutex on it,  
but not actual coherency (see below handle_nr_rach() assumes data is up-to-date)  
The first part (in NR_UL_indication, uses the data computed by the lower part (phy_procedures_gNB_uespec_RX), but for the **previous** slot  
Then, phy_procedures_gNB_uespec_RX will hereafter replace the data for the next run  
This is very tricky and not thread safe at all.
{: .func3}

#### NR_UL_indication()  
This block processes data already decoded and stored in structures behind UL_INFO
{: .func4}

* handle_nr_rach()  
process data from RACH primary detection  
if the input is a UE RACH detection
{: .func4}
    * nr_schedule_msg2()
{: .func4}
* handle_nr_uci()  
????	      
{: .func4}
* handle_nr_ulsch()  
handles ulsch data prepared by nr_fill_indication()
{: .func4}
* gNB_dlsch_ulsch_scheduler ()  
also calls "run_pdcp()", as this is not a autonomous thread, it needs to be called here to update traffic requests (DL) and to propagate waiting UL to upper layers  
Calls schedule_nr_mib() that calls mac_rrc_nr_data_req() to fill MIB,  
Calls each channel allocation: schedule SI, schedule_ul, schedule_dl, ...  
this is a major entry for "phy-test" mode: in this mode, the allocation is fixed  
all these channels goes to mac_rrc_nr_data_req() to get the data to transmit
{: .func4}
* NR_Schedule_response()  
process as per the scheduler decided
{: .func4}

#### L1_nr_prach_procedures()  
????
{: .func4}
#### phy_procedures_gNB_uespec_RX()
* nr_decode_pucch0()  
actual CCH channel decoding  
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
{: .func4}
populated the data for the next call to "NR_UL_indication()"
{: .func4}

#### phy_procedures_gNB_TX()
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


<div class="panel panel-info">
**Note**
{: .panel-heading}
<div class="panel-body">

NOTE DESCRIPTION

</div>
</div>

