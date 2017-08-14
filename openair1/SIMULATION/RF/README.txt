This directory holds the C language implementation of the
RF impairments simulation.  It contains
the following files:

defs.h         : Defines function prototypes and data structures
rf.c           : routines for generation of random RF impairments 
Makefile       : Makefile for standalone generation of channel

See Doxygen output for API description
 
Compilation Variables
	    RF_MAIN : Compile the code with a test main entry point.  This mode allows 
			   the user to generate to generate impairments for a sinusoidal input and generate
			   output testing data to analyze results in MATLAB/OCTAVE. The command line 
			   arguments are:
			   
			   rf 

			       test_output_re.m :  Real part of output
					           Variable is rfout_re
			       test_output_im.m :  Imaginary part of output
						   Variable is rfout_im
