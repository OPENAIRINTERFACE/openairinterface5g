This directory contains files implementing RS,DRS and SRS generation along with the LTE gold sequence generator

lte_dl_cell_spec.c  : DL RS signal generation for TX and RX (channel estimation) 
lte_gold.c          : Generators for LTE gold sequences (used for pilots and scrambling)
lte_ul_ref.c        : UL DRS signal generation for TX and RX
defs.h              : data structures and function prototypes
mod_table.h         : lookup tables for QAM modulation (CBMIMO1 tables)
primary_synch.m     : MATLAB/OCTAVE generator for PSS
gen_mod_table.m     : MATLAB/OCTAVE generator for CBMIMO1 mod_table.h
lte_ul.m            : MATLAB/OCTAVE generator for UL (SC-FDMA) DFT sizes                           
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~                                                                                                                               
~          
