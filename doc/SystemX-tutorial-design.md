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
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --parallel-config PARALLEL\_SINGLE\_THREAD
```

**nrUE**

```bash
sudo ./nr-uesoftmodem --numerology 1 -r 106 -C 3510000000 -d --rfsimulator.serveraddr 127.0.0.1
```
