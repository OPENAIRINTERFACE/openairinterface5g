<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">Running OAI Basic Simulator</font></b>
    </td>
  </tr>
</table>

This page is valid on the following branches:

- `master` starting from tag `v1.1.0`
- `develop` starting from tag `2019.w11`

# 1. Building the basic-simulator.

After the build simplification, the basic simulator is available directly from the standard build.

```bash
$ source oaienv
$ cd cmake_targets
$ ./build_oai --eNB --UE
```

Both eNB (lte-softmodem) and UE (lte-uesoftmodem) are present on `cmake_targets/ran_build/build` folder.

More details are available on the [build page](BUILD.md).

# 2. Running the basic simulator.

The basic simulator is a oai device replacing the radio heads (for example the USRP device). It allows connecting the oai UE and the oai eNodeB through a network interface carrying the time-domain samples, getting rid of over the air unpredictable perturbations.

This is the ideal tool to check signal processing algorithms and protocols implementation and having debug sessions without any HW radio equipment.

The main limitations are:

- A single OAI UE will connect to the OAI eNB
- No channel noise

## 2.1. Starting eNB

The basic simulator is able to run with a connected EPC or without any (the so-called "noS1" mode).

Example 1: running in FDD mode with EPC.

```bash
$ source oaienv
$ cd cmake_targets/ran_build/build
$ ENODEB=1 sudo -E ./lte-softmodem -O $OPENAIR_HOME/ci-scripts/conf_files/lte-fdd-basic-sim.conf --basicsim
```

Edit previously the `ci-scripts/conf_files/lte-fdd-basic-sim.conf` file to modify:

- `N_RB_DL` field to change the Bandwidth (25, 50, 100)
- `CI_MME_IP_ADDR` with the EPC IP address
- `CI_ENB_IP_ADDR` with the container (physical server, virtual machine, ...) on which you are executing the eNB soft-modem

Example 2: running in TDD mode without any EPC.

```bash
$ source oaienv
$ cd cmake_targets/ran_build/build
$ ENODEB=1 sudo -E ./lte-softmodem -O $OPENAIR_HOME/ci-scripts/conf_files/lte-tdd-basic-sim.conf --basicsim --noS1
```

## 2.2. Starting UE

Before starting the UE, you may need to edit the SIM parameters to adapt to your eNB configuration and HSS database.

The <conf> file to use for conf2uedate is `openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf`

You need to set the correct OPC, USIM_API_K, MSIN (this is the end par of the IMSI), HPLMN (the front part of IMSI) to match values from HSS.

```bash
$ source oaienv
# Edit openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf
$ cd cmake_targets/ran_build/build
$ ../../nas_sim_tools/build/conf2uedata -c $OPENAIR_HOME/openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o .
$ sudo -E ./lte-uesoftmodem -C 2625000000 -r 25 --ue-rxgain 140 --basicsim [--noS1]
```

The `-r 25` is to use if in the conf file of the eNB you use N_RB_DL=25. Use 50 if you have N_RB_DL=50 and 100 if you have N_RB_DL=100.

The `-C 2625000000` is the downlink frequency. Use the same value as `downlink_frequency` in the eNB configuration file.

The `--noS1` is mandatory if you started the eNB in that mode.

# 3. Testing the data plane

# 3.1. In S1 mode

First we need to retrieve the IP address allocated to the OAI UE.

On the server that runs the UE:

```bash
$ ifconfig oaitun_ue1
oaitun_ue1 Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  
            inet addr:192.172.0.2  P-t-P:192.172.0.2  Mask:255.255.255.0
...
```

`192.172.0.2` is the IP address that has been allocated by the SPGW in the EPC.

On the server that runs the EPC:

```bash
$ ping -c 20 192.172.0.2
  --- 192.172.0.2 ping statistics ---
  20 packets transmitted, 20 received, 0% packet loss, time 19020ms
  rtt min/avg/max/mdev = 13.241/18.999/24.208/2.840 ms
```

You can ping the EPC from the UE:

```bash
$ ping -I oaitun_ue1 -c 20 192.172.0.1
  --- 192.172.0.1 ping statistics ---
...
  20 packets transmitted, 20 received, 0% packet loss, time 19019ms
  rtt min/avg/max/mdev = 13.015/18.674/23.738/2.917 ms
```

For DL iperf testing:

On the server that runs the UE.

```bash
$  iperf -B 192.172.0.2 -u -s -i 1 -fm -p 5001
```

On the server that runs the EPC.

```bash
$  iperf -c 192.172.0.2 -u -t 30 -b 10M -i 1 -fm -B 192.172.0.1 -p 5001
```

For UL iperf testing:

On the server that runs the EPC.

```bash
$  iperf -B 192.172.0.1 -u -s -i 1 -fm -p 5001
```

On the server that runs the UE.

```bash
$  iperf -c 192.172.0.1 -u -t 30 -b 2M -i 1 -fm -B 192.172.0.2 -p 5001
```

# 3.2. In noS1 mode

The IP addresses are fixed. But we can still retrieve them programmatically.

For the UE it is quite the same:

```bash
$ ifconfig oaitun_ue1
oaitun_ue1 Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  
            inet addr:10.0.1.2  P-t-P:10.0.1.2  Mask:255.255.255.0
...
```

For the eNB:

```bash
$ ifconfig oaitun_enb1
oaitun_enb1 Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  
            inet addr:10.0.1.1  P-t-P:10.0.1.1  Mask:255.255.255.0
...
```

Pinging like this:

```bash
$ ping -I oaitun_ue1 -c 20 10.0.1.1
$ ping -I oaitun_enb1 -c 20 10.0.1.2
```

And the same for iperf:

```bash
$ iperf -B 10.0.1.2 -u -s -i 1 -fm
$ iperf -c 10.0.1.2 -u -b 1.00M -t 30 -i 1 -fm -B 10.0.1.1
```
