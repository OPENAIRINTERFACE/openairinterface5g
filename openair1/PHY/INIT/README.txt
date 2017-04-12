This directory contains files related to initialization of variables/structures

init_top.c : initialize top-level variables and signal buffers, FFT twiddle factors, etc.
lte_init.c : LTE specific initlization routines (DLSCH/ULSCH signal buffers for RX, data buffers for TX, etc.)
lte_param_init.c: used only in unitary simulations. initializes the global variables eNB and UE.
lte_parms.c: contains init_frame_parms to initialize frame parameters structure

