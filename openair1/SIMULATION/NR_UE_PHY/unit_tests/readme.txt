/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

File: /home/jacques/workspace/oai_openairinterface/openairinterface5g/openair1/SIMULATION/NR_UE_PHY/unit_tests/README.txt

This directory contains testbenchs for 5G NR physical layers.

Before building unit tests, full UE build should be done before (maily for asn1 dependencies):

Initialise environment:
- under openairinterface5g enter >source oaienv
Install tools:
- under openairinterface5g/cmake_targets/build -I
Build NR UE:
- under openairinterface5g/cmake_targets/build --nrUE

Specific test files 
====================

pss_test.c: test for detection of primary synchronisation signal.             
sss_test.c: test for detection of secundary synchronisation signal.
pbch_test.c: test for decoding of packet braodcast channel.
srs_test.c: test of sounding reference signal.
frame_config_test.c : test of frame configurations (FDD/TDD).
harq_test.c : test of HARQ downlink and uplink.
pucch_uci_test : test of packed Uplink Control Channel Uplink Control Information
pss_util_test.c:  common functions for running synchronisation tests.
pss_util_test.h: common headers for synchronisation tests.
input_buffer_test.h: it allows providing samples for pss test.

How to build & run tests
========================

Before building, oai environment should be set by entering under directory openairinterface5g "source oaienv".

A script "run_tests.sh" allows to build, to run and to check all tests.
To run a complete non regression of unit tests, go under directory NR_UE_PHY/unit_tests/build and execute:
./run_tests.sh

This script "run_tests.sh" list all available unit tests.

build is based on cmake tool.

cmake file is "/openairinterface5g/openair1/SIMULATION/NR_UE_PHY/unit_tests/build/CMakeList.txt".

CMakeList.txt lists different build.

How to build: in directory "oai_openairinterface/openairinterface5g/openair1/SIMULATION/NR_UE_PHY/unit_tests/build/", below commands should be entered:

First command to do:
cmake CMakeList.txt     -> it generates makefiles to build all projects.

Then build of test:
make pss_test           -> build pss_test: detection of NR (Primary Synchronisation Channel - synchronisation of UE).
make sss_test           -> build sss_test: detection of NR SSS (Secundary Synchronisation Channel - second step for UE synchronisation).
make pbch_test          -> build pbch_test: decoding of NR PBCH (Packet Broadcast Channel -> UE read MIB Master Information Block which gives network parameters).
make frame_config_test  -> build frame_config_test: NR FDD/TDD configuration.
make srs_test           -> build srs_test: Sounding Reference Signals are transmitted by UE to the network which can use them for channel estimations.
make harq_test          -> build harq_test: Hybrid Repeat Request Acknowledgment: mecanism to acknowledge transmitted and received transport blocks.
make pucch_uci_test     -> build pucch_uci_test : Packet Uplink Control Channel / Uplink Control information : create UCI payload and select PUCCH formats and its parameters. 
make clean              -> clean all projects.

And execution of tests:
./pss_test              -> run NR PSS tests.
./sss_test              -> run NR SSS tests.
./pbch_test             -> run NR PBCH tests.
./srs_test              -> run NR srs tests.
./frame_config_test     -> run NR frame configuration tests.
./harq_test             -> run NR HARQ downlink and uplink tests.
./pucch_uci_test        -> run NR PUCCH UCI tests.

There is a script in build/run_test.sh which allows:
- building/running/checking all tests

What is test result?
====================

Test verdict consists in a comparaison of generated logging file at execution with a reference logging file which is stored under git for each test.
By default, tool "meld" is open each time there is a difference between reference file and logging file.
It is possible to disable execution of meld with option -m. 

Build/execute only one test:
===========================

The script "run_tests.sh" has different options:
"-b : No Build of unit tests"
"-c : No check for unit test"
"-e : No execution of unit tests"
"-m : No run of meld tool"

./run_tests.sh                       -> run all unit tests (build, execute and compare for each test)
./run_tests.sh -m                    -> run all unit tests but meld is not used (verdict is based on system command "cmp").
./run_tests.sh -b -c -r -m           -> list all available unit tests
./run_tests.sh pss_test              -> run only one test (here it is pss_test).
./run_tests.sh -c -e -m pss_test     -> build only one test
./run_tests.sh -m pss_test         -> run only one test but without execution of meld

Which processing can be tuned?
=============================

Decimation feature: 

By default pss processing uses samples from received buffer at input sampling frequency fs.
For saving processing time, it is possible that pss uses a decimation of input signal.
To enable decimation file openairinterface5g_bis/openairinterface5g/openair1/PHY/NR_REFSIG/pss_nr.h should be modified.
//#define PSS_DECIMATOR should be enable to get decimation.
By default, decimation uses a FIR filter.
It is possible to use a CIC (Cascaded Integrated Comb filter) for decimation by enabling line //#define CIC_DECIMATOR.

Input received signal:

Different waveforms can be generated for testing pss and sss detection.
By default, it uses a sinusoidal signals at different frequencies with an amplitude at the same level than pss or sss signals.
For modifying these waveforms, function "set_random_rx_buffer" in file pss_util_test.c has to be modified.
For wavecom type, line data_format = SINUSOIDAL_DATA should be changed with new format.
For amplitude of waveform, declaration have to be modified in the function.

Status
======

NR synchronisation tests run successfully with default parameters.
Warning: Tests of PSS NR with decimation (based on FIR or CIC filter) does not run successfully at the moment. 



