# OpenAirInterface for SystemX

# Terminology

****This document use the 5G terminology****

**Central Unit (CU):** It is a logical node that includes the gNB
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

# OpenAirUsage

## EPC and general environment

### OAI EPC

Use the stable OAI EPC, that can run in one machine (VM or standalone)

Draft description:
<https://open-cells.com/index.php/2017/08/22/all-in-one-openairinterface-august-22nd/>

## Standalone 4G

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

```bash
ip netns delete aNameSpace 2&gt; /dev/null

ip link delete v-eth1 2&gt; /dev/null

ip netns add aNameSpace

ip link add v-eth1 type veth peer name v-peer1

ip link set v-peer1 netns aNameSpace

ip addr add 10.200.1.1/24 dev v-eth1

ip link set v-eth1 up

iptables -t nat -A POSTROUTING -s 10.200.1.0/255.255.255.0 -o enp0s31f6 \
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

```bash
ip netns exec aNameSpace bash
```

The traffic genenrator has to specify the interface:

```bash
route add default oaitun_ue1
```

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

 `targets/RT/USER/lte-ru.c and targets/RT/USER/lte-enb.c`

As this piece of SW is very complex and doesn’t meet our goals
(functional split 6), a cleaned version replaces these 2 files in
executables/ocp-main.c (openair1/SCHED/prach\_procedures.c is also
replaced by this new file as it only launching the RACH actual work in a
way not compatible with our FS6).

The main loop cadences the I/Q samples reception, signal processing and
I/Q samples sending.

The main loop uses extensively function pointers to call the right
processing function depending on the split case.

A lot of OAI reduntant global variables contains the same semantic data: time,frame, subframe.
The reworked main loop take care of a uniq variable that comes directly from harware: RF board sampling number.

To use OAI, we need to set all OAI variables that derivates from this timestamp value. The function setAllfromTS() implements this.

### Splitted main level

When FS6 is actived, a main loop for DU (du_fs6()) a main loop for CU case replaces the uniq eNB main loop.

Each of these main loops calls initialization of OAI LTE data and the FS6 transport layer initialization.

Then, it runs a infinite loop on: set time, call UL and DL. The time comes from the RF board, so the DU sends the time to the CU.

This is enough for RF board dialog, but the FS6 is higher in SW layers,
we need to cut higher functions inside downlink and uplink procedures.

As much as possible, the FS6 code is in the directory OPENAIR_DIR/executables. When a given OAI piece of code is small or need complex changes, it is reworked in the file fs6-main.c. The functions naming keeps the OAI function name, adding suffix _fromsplit() or _tosplit().

When this organization would lead to large code copy, it is better to insert modifications in OAI code. This is done in two files: 

- openair1/SCHED/phy_procedures_lte_eNb.c: to send signaling channels computation results
    - the function sendFs6Ulharq() centralizes all signaling channels forwarding to CU
- openair1/PHY/LTE_TRANSPORT/ulsch_decoding.c: to deal with FS6 user plane split
    - sendFs6Ul() is used once to forward user plane to CU


### DownLink

The main procedure is phy\_procedures\_eNB\_TX()

This is building the common channels (beacon, multi-UE signaling).

The FS6 split breaks this function into pieces:

*   The multi-UE signals, built by common\_signal\_procedures(),
    subframe2harq\_pid(), generate\_dci\_top(), subframe2harq\_pid()
    *   These functions run in the DU, nevertheless all  context has to be sent 
    (it is also needed partially for UL spitting)
    * Run in the DU also to meet the requirement of pushing
        in DU the data encoded with large redundancy (&gt;3 redundancy)
        
*   the per UE data: pdsch\_procedures() needs further splitting:

    *   dlsch\_encoding\_all() that makes the encoding: turbo code
        and lte\_rate\_matching\_turbo() that will be in the DU (some
        unlikely cases can reach redundancy up to x3, when MCS is very
        low (negative SINR cases)).

        *   dlsch\_encoding() output needs to be transmitted between the
            DU and the CU for functional split 6.
            * dlsch\_scrambling() that will go in the DU
            * dlsch\_modulation() that will go in the DU
   
   The du user plane data is made of expanded bit in OAI at FS6 split level. 1 pair of functions compact back these bits into 8bits/byte before sending data and expand it again in the DU data reception (functions: fs6Dl(un)pack()).

### Uplink

The uplink require configuration that is part of the DL transmission.

It interprets the signalling to extract the RACH and the per UE data
channels.

Ocp-main.c:rxtx() calls directly the entry procedure
phy\_procedures\_eNB\_uespec\_RX() calls:

*   rx\_ulsch() that demodulate and extract soft bits per UE.

    *   This function runs in the DU
    *   the output data will be processes in the DU, so it needs to be
        transmitted to the DU
*   ulsch\_decoding() that do lte\_rate\_matching\_turbo\_rx()
    sub\_block\_deinterleaving\_turbo() 
    then turbo decode that is in the CU
*   fill\_ulsch\_cqi\_indication()  fill\_crc\_indication() , fill\_rx\_indication()
          *   DU performs the signal processing of each channel data, prepare and sent to the CU the computed result

* Random access channel detection runs in the DU
      * the DU reports to the CU only the detected temprary identifier for RACH response


### signaling data in each direction (UL and DL)


*   each LTE channel needs to be propagated between CU and DU
    * the simplest are the almost static data such as PSS/SSS, that need only static eNB parameters and primary information (frame numbering)
    * all the other channels require data transmission CU to DU and DU to CU
    * the general design push all the low level processing for these channels in the DU 
    * the CU interface transports only signal processing results (UL) or configuration to create the RF signal (DL case)
* HARQ is detected in the DU, then only the ACK or NACK is reported to CU

* the CU have to control the power and MCS (modulation and coding scheme)
    * the DU performs the signal processing and report only the decoded data like the CQI
  * as the DU performas the modulation, scrambling and puncturing, each data packet is associated with the LTE parameters required for these features
       * in DL, the CU associates the control parameters and the user plane data
       * in UL, the CU sends upfront the scheduled UL data to the DU.  So, the DU have the required knowledge to decode the next subframes in time.

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

The end line option “--split73” enables the fs6 (also called split 7.3) mode and decided to be cu or du.

Example:

```bash
./ocp-softmodem -O $OPENAIR_DIR/enb.fs6.example.conf --rfsim  --log_config.phy_log_level debug --split73 cu:127.0.0.1
```

Run the CU init of the split 6 eNB, that will call du on 127.0.0.1 address

```bash
./ocp-softmodem -O $OPENAIR_DIR/enb.fs6.example.conf --rfsim  --log_config.phy_log_level debug --split73 du:127.0.0.1
```

will run the du, calling the cu on 127.0.0.1

If the CU and the DU are not on the same machine, the remote address of each side need to be specified as per this example

```bash
./ocp-softmodem -O $OPENAIR_DIR/enb.fs6.example.conf --rfsim  --log_config.phy_log_level debug --split73 du:192.168.1.55
```

runs the functional split 6 DU

```bash
./lte-uesoftmodem -C 2685000000 -r 50 --rfsim --rfsimulator.serveraddr 192.168.1.1 -d
```

Runs the UE (to have the UE signal scope, compile it with make uescope)

CU+DU+UE can run with option `--noS1` to avoid to use a EPC and/or with `--rfsim` to simulate RF board


## 5G and F1

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

**gNB**

```bash
sudo RFSIMULATOR=server ./nr-softmodem -O \
../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf \
--parallel-config PARALLEL\_SINGLE\_THREAD
```

**nrUE**

```bash
sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --numerology 1 -r 106 -C \
3510000000 -d
```
