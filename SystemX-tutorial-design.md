# OpenAirInterface for SystemX

#Terminology

****This document use the 5G terminology****

**Central Unit (CU): **It is a logical node that includes the gNB
functions like Transfer of user data, Mobility control, Radio access
network sharing, Positioning, Session Management etc., except those
functions allocated exclusively to the DU. CU controls the operation of
DUs over front-haul (Fs) interface. A central unit (CU) may also be
known as BBU/REC/RCC/C-RAN/V-RAN/VNF

**Distributed Unit (DU):** This logical node includes a subset of the
gNB functions, depending on the functional split option. Its operation
is controlled by the CU. Distributed Unit (DU) also known with other
names like RRH/RRU/RE/RU/PNF.

In OpenAir code, the terminology is often RU and BBU.

#Usage
##EPC and general environment
### OAI EPC

Use the stable OAI EPC, that can run in one machine (VM or standalone)

Draft description:
<https://open-cells.com/index.php/2017/08/22/all-in-one-openairinterface-august-22nd/>

##Standalone 4G

EPC+eNB on one machine, the UE can be commercial or OAI UE.
### USRP B210

Main current issue: traffic is good only on coaxial link between UE and
eNB (probably power management issue).

### Simulated RF

Running eNB+UE both OAI can be done over a virtual RF link.

The UE current status is that threads synchronization is implicit in
some cases. As the RF simulator is very quick, a “sleep()” is required
in the UE main loop

(line 1744, targets/RT/USER/lte-ue.c).

Running also the UE in the same machine is possible with simulated RF.

Running in same machine is simpler, offers about infinite speed for
virtual RF samples transmission.

A specific configuration is required because the EPC Sgi interface has
the same IP tunnel end point as the UE.

So, we have to create a network namespace for the UE and to route data
in/out of the namespace.
``` bash
ip netns delete aNameSpace 2&gt; /dev/null

ip link delete v-eth1 2&gt; /dev/null

ip netns add aNameSpace

ip link add v-eth1 type veth peer name v-peer1

ip link set v-peer1 netns aNameSpace

ip addr add 10.200.1.1/24 dev v-eth1

ip link set v-eth1 up

iptables -t nat -A POSTROUTING -s 10.200.1.0/255.255.255.0 -o enp0s31f6
-j MASQUERADE

iptables -A FORWARD -i enp0s31f6 -o v-eth1 -j ACCEPT

iptables -A FORWARD -o enp0s31f6 -i v-eth1 -j ACCEPT

ip netns exec aNameSpace ip link set dev lo up

ip netns exec aNameSpace ip addr add 10.200.1.2/24 dev v-peer1

ip netns exec aNameSpace ip link set v-peer1 up

ip netns exec aNameSpace bash
```

After the last command, the Linux shell is in the new namespace, ready
to run the UE.

To make user plan traffic, the traffic generator has to run in the same
namespace

`ip netns exec aNameSpace bash
`

The traffic genenrator has to specify the interface:

`route add default oaitun_ue1
`

or specify the outgoing route in the traffic generator (like option “-I”
in ping command).

## Split 6 DL 4G

The contract describes to reuse the uplink existing if4p5 and to develop
is this work the downlink “functional split 6”.

The customer required after signature to develop also the uplink
functional split 6. This is accepted, as long as the whole work is
research with no delivery completeness warranty.
### Simulation

To be able to verify the new features and to help in all future
developments, Open Cells added and improved the Rf board simulator
during this contract.

We added the channel modeling simulation, that offer to simulate various
3GPP defined channels.
### Main loop

The main log is in RF simulator is in

 targets/RT/USER/lte-ru.c and targets/RT/USER/lte-enb.c

As this piece of SW is very complex and doesn’t meet our goals
(functional split 6), a cleaned version replaces these 2 files in
executables/ocp-main.c (openair1/SCHED/prach\_procedures.c is also
replaced by this new file as it only launching the RACH actual work in a
way not compatible with our FS6).

The main loop cadences the I/Q samples reception, signal processing and
I/Q samples sending.

The main loop uses extensively function pointers to call the right
processing function depending on the split case.

This is enough for RF board dialog, but the FS6 is higher in SW layers,
we need to cut higher functions inside downlink and uplink procedures.

### DownLink

The main procedure is phy\_procedures\_eNB\_TX

This is building the common channels (beacon, multi-UE signaling).

The FS6 split breaks this function into pieces:

-   The multi-UE signals, built by common\_signal\_procedures(),
    subframe2harq\_pid(), generate\_dci\_top(), subframe2harq\_pid()

    -   These functions will be executed in the DU, nevertheless all
        context has to be sent (it is also needed partially for
        UL spitting)
    -   IT should be in the DU also to meet the requirement of pushing
        in DU the data encoded with large redundancy (&gt;3 redundancy)
-   the per UE data: pdsch\_procedures() that needs further splitting:

    -   dlsch\_encoding\_all() that makes the encoding: turbo code
        and lte\_rate\_matching\_turbo() that will be in the DU (some
        unlikely cases can reach redundancy up to x3, when MCS is very
        low (negative SINR cases)).

        -   dlsch\_encoding() output needs to be transmitted between the
            DU and the CU for functional split 6.
    -   dlsch\_scrambling() that will go in the DU
    -   dlsch\_modulation() that will go in the DU

### Uplink

The uplink require configuration that is part of the DL transmission.

It interprets the signalling to extract the RACH and the per UE data
channels.

Ocp-main.c:rxtx() calls directly the entry procedure
phy\_procedures\_eNB\_uespec\_RX() calls:

-   rx\_ulsch() that demodulate and extract soft bits per UE.

    -   This function runs in the DU
    -   the output data will be processes in the DU, so it needs to be
        transmitted to the DU
-   ulsch\_decoding() that do lte\_rate\_matching\_turbo\_rx()
    sub\_block\_deinterleaving\_turbo() then turbo decode

    -   it runs
-   fill\_ulsch\_cqi\_indication()

    -   TBD: either runs in DU or output needs to be transmitted to DU
-   fill\_crc\_indication() , fill\_rx\_indication() results need also
    to be transmitted or made in the DU

### UDP transport layer

A general UDP transport layer is in executables/transport\_split.c

Linux offers a UDP socket builtin timeout, that we use.

In and out buffers are memory zones that contains compacted
(concatenated) UDP chunks.

For output, sendSubFrame() sends each UDP chunk

For input, receiveSubFrame() collects all UDP chunks for a group (a
subframe in OAI LTE case). It returns in the following cases:

-   all chunks are received
-   a timeout expired
-   a chunk from the next subframe already arrived

### Functional split 6 usage

The ocp cleaned main hale to be used: run ocp-softmodem instead of
lte-softmodem.

The functionality and parameters is the same, enhanced with FS6 mode.

The environment variable “fs6” enables the fs6 mode and decided to be cu
or du from the fs6 variable content.

A example configuration file is: $OPENAIR_DIR/enb.fs6.example.conf
The IP addresses in this file should be updated for each network.

Example:

`fs6=cu ./ocp-softmodem -O $OPENAIR_DIR/enb.fs6.example.conf --rfsim  --log_config.phy_log_level debug
`

Run the CU init of the split 6 eNB.

`fs6=du ./ocp-softmodem -O $OPENAIR_DIR/enb.fs6.example.conf --rfsim  --log_config.phy_log_level debug
`

runs the functional split 6 DU

`./lte-uesoftmodem -C 2685000000 -r 50 --rfsim --rfsimulator.serveraddr 192.168.1.1 -d
`

Runs the UE (to have the UE signal scope, compile it with make uescope)

CU and DU IP address and port are configurable in the eNB configuration
file (as X2, GTP, … interfaces).

##5G and F1

Today 5G achievement is limited to physical layer.

The available modulation is 40MHz, that require one X310 or N300 for the
gNB and a X310 or N300 for the nrUE.

### Usage with X310

Linux configuration:
<https://files.ettus.com/manual/page_usrp_x3x0_config.html>

We included most of this configuration included in OAI source code.

Remain to set the NIC (network interface card) MTU to 9000 (jumbo
frames).

### Running 5G

Usage with RFsimulator:

gNB

`sudo RFSIMULATOR=server ./nr-softmodem -O
../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf
--parallel-config PARALLEL\_SINGLE\_THREAD`

nrUE

`sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --numerology 1 -r 106 -C
3510000000 -d`
