measur command can be used to get cpu usage or signal processing statistics from a running eNB instance.


Measurments can be displayed by groups, the list of available groups can be retrieved using the `measur show groups` command.
```bash
softmodem> measur show groups
                           name       type 
00                           enb   ltestats 
01                        enbues   ltestats 
02                           rlc   ltestats 
03                        phycpu   cpustats 
04                        maccpu   cpustats 
05                       pdcpcpu   cpustats 
softmodem>
```
By default the oai softmodem doesn't compute cpu usage measurments, a specific command allows to enable them.
```bash
softmodem> measur cpustats enable
softmodem> measur show phycpu
--------------------------------- cpu (4 GHz) measurements: PHY (cpustats enabled) ---------------------------------
00                   phy_proc_tx:           40.217 us;             178   | 01                   phy_proc_rx:            0.000 us;               0 |
02                      rx_prach:           40.123 us;               9   | 03                      ofdm_mod:            0.000 us;               0 |
04          dlsch_common_and_dci:           39.311 us;             178   | 05             dlsch_ue_specific:            0.878 us;             178 |
06                dlsch_encoding:            5.667 us;              11   | 07              dlsch_modulation:            5.398 us;              11 |
08              dlsch_scrambling:            1.398 us;              11   | 09           dlsch_rate_matching:            1.367 us;              11 |
10        dlsch_turbo_encod_prep:            0.000 us;               0   | 11        dlsch_turbo_encod_segm:            0.000 us;               0 |
12             dlsch_turbo_encod:            2.408 us;              11   | 13     dlsch_turbo_encod_waiting:            0.000 us;               0 |
14      dlsch_turbo_encod_signal:            0.000 us;               0   | 15        dlsch_turbo_encod_main:            0.000 us;               0 |
16     dlsch_turbo_encod_wakeup0:            0.000 us;               0   | 17     dlsch_turbo_encod_wakeup1:            0.000 us;               0 |
18            dlsch_interleaving:            0.503 us;              11   | 19                        rx_dft:            0.000 us;               0 |
20      ulsch_channel_estimation:            0.000 us;               0   | 21  ulsch_freq_offset_estimation:            0.000 us;               0 |
22                ulsch_decoding:            0.000 us;               0   | 23            ulsch_demodulation:            0.000 us;               0 |
24         ulsch_rate_unmatching:            0.000 us;               0   | 25          ulsch_turbo_decoding:            0.000 us;               0 |
26          ulsch_deinterleaving:            0.000 us;               0   | 27          ulsch_demultiplexing:            0.000 us;               0 |
softmodem> measur cpustats disable
```

signal processing statistics are always available.
```bash
softmodem>  measur show enb   
--------------------------------- eNB 0 mac stats CC 0 frame 68 ---------------------------------
           total_num_bcch_pdu =              44                     bcch_buffer =              17               total_bcch_buffer =             937
                     bcch_mcs =               2              total_num_ccch_pdu =               0                     ccch_buffer =               0
            total_ccch_buffer =               0                        ccch_mcs =               0              total_num_pcch_pdu =               0
                  pcch_buffer =               0               total_pcch_buffer =               0                        pcch_mcs =               0
             num_dlactive_UEs =               0                  available_prbs =              25            total_available_prbs =           16924
              available_ncces =               0                   dlsch_bitrate =               0                  dlsch_bytes_tx =               0
                dlsch_pdus_tx =               0             total_dlsch_bitrate =               0            total_dlsch_bytes_tx =               0
          total_dlsch_pdus_tx =               0                   ulsch_bitrate =               0                  ulsch_bytes_rx =               0
                ulsch_pdus_rx =               0             total_ulsch_bitrate =               0            total_ulsch_bytes_rx =               0
          total_ulsch_pdus_rx =               0                 sched_decisions =               0                missed_deadlines =               0
```
[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
