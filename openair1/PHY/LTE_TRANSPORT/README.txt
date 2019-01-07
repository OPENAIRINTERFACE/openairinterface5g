This directory contains functions related to physical/transport channel processing:

defs.h               : function prototypes and data structures for DLSCH/ULSCH/DCI/UCI  processing
vars.h               : some global variables (cqi format tables and SINR thresholds)
dlsch_coding.c       : routines pertaining to the channel coding procedures for dlsch (36-212)
dlsch_decoding.c     : routines for channel decoding/deinterleaving/rate (de)matching of dlsch
dlsch_modulation.c   : routines for the modulation procedures for PDSCH/dlsch (36-211) 
dlsch_demodulation.c : routines for demodulation procedures for PDSCH/dlsch (note: SSE/SSE2/SSE3/SSE4 optimised)
dlsch_scrambling.c   : routines for scrambling/unscrambling of PDSCH (36-211)
pilots.c             : routines pertaining to the generation of cell-specific pilots
drs_modulation.c     : routines for generation of uplink reference signals (DRS) (36-211)
group_hopping.c      : routines for generation of group hopping sequences (incomplete) (36-211)
lte_mcs.c            : routines for computation of mcs/tbs related information (36-212,36-213)
lte_transport_init.c : some initialization routines for DCI structures (simulation/testbench only) 
pbch.c               : routines for encoding/decoding and modulation/demodulation for PBCH (36-211,36-212) note: SSE/SSE2/SSE3/SSE4 optimized
pcfich.c             : routines for encoding/decoding and configuration of PCFICH (36-211)
phich.c              : routines for encoding/decoding and configuration of PHICH (36-211,36-213)
print_stats.c        : routines for DLSCH/ULSCH statistics and CQI/PMI information
pss.c                : routines for generation of PSS (36-211)
pucch.c		     : routines for generation/detection of PUCCH (36-211)
rar_tools.c          : routines for generating ulsch/dlsch modulation/demodulation and coding/decoding structures from a RAR pdu (for use only with openair2)
dci.h                : Data structures for LTE dci formats 0,1,1A,1B,1C,1D,2,2A,2B,3 (36-212)
dci.c                : Top-level encoding/decoding and modulation/demodulation routines for PDCCH/DCI (36-211,36-212) note: SSE/SSE2/SSE3/SSE4 optimized
                     : Search procedure routines for DCI are included (36-213)
dci_tools.c          : tools for generating dlsch/ulsch modulation/demodulation and coding/decoding structures from DCI primitives
ulsch_coding.c       : Top-level routines for ulsch encoding (36-212)
ulsch_decoding.c     : routines for ulsch decoding (36-212)
ulsch_modulation.c   : routines for PUSCH/ulsch modulation (36-211)
ulsch_demodulation.c : routines for PUSCH/ulsch demodulation (note SSE/SSE2/SSE3/SSE4 optimised)
srs_modulation.c     : routiens for generation of SRS (36-211,36-213)  
uci_tools.c          : routines for generation of UCI information in PUSCH

