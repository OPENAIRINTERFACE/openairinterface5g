This folder contains the software for layer 1 of the OpenAirInterface eNB and UE as well as the unitary simulators

|-- PHY			   // contains all signal processing related to physical layer (used in real-time AND simulation)

General remarks: every directory contains at least defs.h (for declaration of structure and functions), vars.h (for definitions of variables), and extern.h (variables are defined only once in xxx-softmodem.c (for real-time HW operation) or a top-level simulation (in SIMULATION/xxx), if variables are needed in another file, use extern.h)

|   |-- CODING		   // turbo and convolutional (de-)coders, rate-matching, crc generation (implementation of 36-212)
|   |   `-- TESTBENCH      // This contains unitary testbenches for the different channel coders/decoders
|   |-- INIT		   // variables defined in vars.h are initialzed here (memory allocation)
|   |-- LTE_ESTIMATION     // estimation for LTE
|   |-- LTE_REFSIG 	   // reference signals for LTE (sync and pilot sequences) reference signals from 36-211 
|   |-- NR_REFSIG 	   // reference signals for NR (sync and pilot sequences) reference signals from 38-211
|   |-- LTE_TRANSPORT      // these are the top level routines for different transport and physical channels (for example DL-SCH, PSS) implements a subset of 36-211,36-212
|   |-- LTE_UE_TRANSPORT      // these are the top level routines for different transport and physical channels (for example DL-SCH, PSS) implements a subset of 36-211,36-212
|   |-- NR_TRANSPORT      // these are the top level routines for different transport and physical channels (for example DL-SCH, PSS) implements a subset of 38-211,38-212
|   |-- NR_UE_TRANSPORT      // these are the top level routines for different transport and physical channels (for example DL-SCH, PSS) implements a subset of 38-211,38-212
|   |-- NBIoT_TRANSPORT      // these are the top level routines for different transport and physical channels for NB-IoT
|   |-- MODULATION	   // modulation and demodulation includes FFT/SC-FDMA front-end processing from 36-211
|   |-- TOOLS		   // tools like (I)FFT, vector operations, matrix multiplication, etc.
|   |   `-- FFTTEST        // unitary testbench for FFT routines
|   |-- defs.h		  // top level defs.h (includes spec_defs, impl_defs and all defs.h of subdirectories)
|   |-- extern.h
|   |-- impl_defs.h       // non-LTE openair implementation
|   |-- impl_defs_lte.h   // lte implementation
|   |-- impl_defs_top.h   // general implementaion defs
|   |-- spec_defs.h
|   |-- spec_defs_top.h
|   |-- types.h
|   `-- vars.h		  
|-- SCHED		  // schedules the different LTE eNB functions
|   |-- defs.h
|   |-- extern.h
|   |-- phy_procedures_lte_eNb.c // LTE PHY procedures for eNB (from 36-213) 
|   |-- phy_procedures_lte_common.c  // LTE PHY procedures common for UE and eNB (from 36-213)
|   |-- prach_procedures.c  //LTE PRACH  procedures (from 36-213)
|   |-- ru_procedures.c  //RU  procedures
|   |-- fapi_l1.c  //L1 side of the FAPI interface
|   |-- phy_mac_stub.c  //MAC stub that generates channels when used in phy-test-mode
|   |-- rt_compat.h
|   `-- vars.h
|-- SCHED_UE		  // schedules the different LTE UE functions
|   |-- phy_procedures_lte_ue.c  // LTE PHY procedures for UE (from 36-213)
|   |-- pucch_pc.c // power control for PUCCH
|   |-- pusch_pc.c // power control for PUSCH
|   |-- srs_pc.c // power control for SRS
|-- SCHED_NB_IOT		  // same as above for NB-IOT
|-- SCHED_NR		  // same as above for NR gNB
|-- SCHED_NR_UE		  // same as above for NR UE
|-- SIMULATION		 // contains simulation routines to test PHY
|   |-- LTE_PHY          // LTE simulation testbenches for unitary simulation of transport/physical channels
|   |   |-- pbchsim.c       // PBCH simulation testbench
|   |   |-- pdcchsim.c      // PCFICH/PHICH/PDCCH (DCI) simulation testbench
|   |   |-- dlsim.c         // PDSCH simulation testbench
|   |   |-- ulsim.c         // PUSCH simulation testbench
|   |   |-- pucchsim.c      // PUCCH simulation testbench
|   |-- RF  		    // RF simulation tools
|   |-- ETH_TRANSPORT
|   `-- TOOLS



