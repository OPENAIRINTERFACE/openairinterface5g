This directory contains files related to parameter estimation

adjust_gain.c                     : gain control loop for UE rx 
lte_dl_channel_estimation.c       : DL channel estimation for LTE RS 
lte_mbsfn_channel_estimation.c	  : DL channel estimation for LTE mbsfn RS 
lte_est_freq_offset.c             : DL frequency offset estimation for LTE RS
lte_ul_channel_estimation.c       : UL channel estimation (both DRS and SRS routines)
freq_equalization.c               : UL frequency-domain equalization for SC-FDMA receiver
lte_dl_channel_estimation_emos.c  : DL channel estimation for EMOS-specific pilot arrangements
lte_sync_time.c                   : DL initial time symncronization based on PSS
lte_adjust_sync_ue.c              : DL timing adjustment (tracking) based on LTE RS-based channel estimates
lte_eNB_measurements.c            : UL measurement procedures
lte_ue_measurements.c             : DL measurement procedures