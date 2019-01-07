PHY/CODING/TESTBENCH

This directory contains a testbench for performance evalulation (antipodal modulation) of the various channel decoding algorithms.

Files:

Makefile                 : Makefile for generating the two testbenches
ltetest.c                : Testbench for LTE Turbo code (rate 1/3 terminated code).  Performs LTE rate-matching 
			   for different mcs.
viterbi_test.c		 : Testbench for 802.11 Viterbi decoder (rate 1/2 terminated code)

pdcch_test.c             : Testbench for LTE PDCCH/DCI encoding/decoding (rate 1/3 Tail-biting code). Performs LTE rate-matching
			   and allows testing different aggregation levels and DCI sizes.
