[![Build Status](https://travis-ci.org/cisco/open-nFAPI.svg?branch=master)](https://travis-ci.org/cisco/open-nFAPI)
[![Coverity Status](https://scan.coverity.com/projects/11791/badge.svg)](https://scan.coverity.com/projects/cisco-open-nfapi)

# open-nFAPI
 
Open-nFAPI is implementation of the Small Cell Forum's network functional API or nFAPI for short. 
nFAPI defines a network protocol that is used to connect a Physical Network Function (PNF) 
running LTE Layer 1 to a Virtual Network Function (VNF) running LTE layer 2 and above. The specification
can be found at http://scf.io/documents/082.
 
The aim of open-nFAPI is to provide an open interface between LTE layer 1 and layer 2 to allow for
interoperability between the PNF and VNF & also to facilitate the sharing of PNF's between
different VNF's

Open-nFAPI implements the P4, P5 and P7 interfaces as defined by the nFAPI specification. 
* The P5 interface allows the VNF to query and configure the 'resources' of the PNF; i.e slice it into
 1 or more PHY instances.
* The P7 interface is used to send the subframe information between the PNF and VNF for a PHY instance
* The P4 interface allows the VNF to request the PNF PHY instance to perform measurements of the surrounding network

The remaining interfaces are currently outside of the scope of this project.

Supports release 082.09.05 of the nFAPI specification

**The Small Cell Forum cordially requests that any derivative work that looks to 
extend the nFAPI libraries use the specified vendor extension techniques, 
so ensuring the widest interoperability of the baseline nFAPI specification 
in those derivative works.**

## Awards

The Open-nFAPI project has won the Small Cell Forum Judges Choice award 2017. (http://www.smallcellforum.org/awards-2/winners-2017/)

## Licensing

The open-nFAPI libraries are release by CISCO under an Apache 2 license. See `LICENSE.md` file for details

## Downloading

The open-nFAPI project can be pulled from git hub

```
git clone https://github.com/cisco/open-nFAPI.git nfapi
```

The following dependencies are required. These are based on a fresh ubuntu installation.

```
sudo apt-get install autoconf
sudo apt-get install gcc
sudo apt-get install g++
sudo apt-get install libtool
sudo apt-get install make
sudo apt-get install doxygen
sudo apt-get install libcunit1-dev
sudo apt-get install libz-dev
sudo apt-get install libsctp-dev
sudo apt-get install libboost-all-dev
```



## Building

To build the open-nFAPI project

```
autoreconf -i
./configure
make
```

To run the unit and integration tests

```
make check
```

You may notice in the console output of the final integration tests the following

```
*** Missing subframe 123 125
```

Out of the box the machine on which you are running has not be configured for real time operation as a result
the vnf may not be scheduled at the correct times and hence it may risk 'missing' subframe opportunities. This
warning indicates this has happened. 

## Running the simulator

The vnf and pnf simulator can be run using the following commands. The pnf and vnf simulator support sourcing and sinking
data over udp. Review the xml configuration files for the details of the port and address to configure. Console logging will show
which address:port is being used

Note : Pinning the simulators to unused cores will produce more consistent behaviour.

Note : You may have to run the processes with sudo to be able to set the real time scheduling and priority

### vnf simulator

To run the vnf simulator you need to specify the port the vnf will listen for p5 connection request upon and also the xml configuration file

```
vnfsim <port> <xml config file>
```

### pnf simulator

To run the vnf simulator you need to specify the addrss & port the pnf will connect to the vnf on and also the xml configuration file

```
pnfsim <address> <port> <xml config file>
```


## Directory structure

```
docs                    doxgen documentation
common                  common code used by the nfapi libraries
nfapi                   the nfapi library including message definitions & encode/decode functions
pnf                     the pnf library for p4, p5, & p7 interfaces
vnf                     the vnf library for p4, p5, & p7 interfaces
sim_common              common simulation for used by the vnf and pnf sim
vnf_sim                 a vnf simulator including a stub mac implementation
pnf_sim                 a pnf simualtor including a fapi interface defintion and stub implementation
xml                     xml configuration files for the vnf and pnf simulator
wireshark               code for a wireshark dissector for the nFAPI protocol
```

## Coverity

Coverity runs on the coverity-scan branch. Changes must be merged to the coverity-scan branch to be checked.


## eNB Integration - Open Air Interface

The open-nFAPI implementation has been integrated with the Open Air Interface solution here (https://gitlab.eurecom.fr/oai/openairinterface5g) and is (at the time of writing) here (https://gitlab.eurecom.fr/daveprice/openairinterface5g/tree/nfapi-ru-rau-split). The open-nFAPI implementation is integrated with the source eNB implementation with any changes required applied as a patch on top of the baseline open-nFAPI library. Any extensions required must be implemented through the vendor extensions as specified by the Small Cell Forum documentation. Any integration wrapping of functionality must be done within the target environment as shown in the Open Air Interface implementation nfapi directory which is defined by the $NFAPI_DIR location at the top level.
