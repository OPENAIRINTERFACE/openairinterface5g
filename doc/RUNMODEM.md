<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">Running OAI Softmodem</font></b>
    </td>
  </tr>
</table>

After you have [built the softmodem executables](BUILD.md) you can set your default directory  to the build directory `cmake_targets/ran_build/build/` and start testing some use cases. Below, the description of the different oai functionalities should help you choose the oai configuration that suits your need. 

# Basic Simulator

See the [dedicated page](BASIC_SIM.md).

# RF Simulator

The rf simulator is a oai device replacing the radio heads (for example the USRP device). It allows connecting the oai UE (LTE or 5G) and respectively the oai eNodeB or gNodeB through a network interface carrying the time-domain samples, getting rid of over the air unpredictable perturbations. This is the ideal tool to check signal processing algorithms and protocols implementation.  The rf simulator has some preliminary support for channel modeling.

It is planned to enhance this simulator with the following functionalities:

- Support for multiple UE connections,each UE being a `lte-uesoftmodem` or `nr_uesoftmodem` instance.
- Support for multiple eNodeB's or gNodeB's for hand-over tests

   This is an easy use-case to setup and test, as no specific hardware is required. The [rfsimulator page](../targets/ARCH/rfsimulator/README.md ) contains the detailed documentation.

# L2 nFAPI Simulator

This simulator connects a eNodeB  and UEs through a nfapi interface, short-cutting the L1 layer. The objective of this simulator is to allow multi UEs simulation, with a large number of UEs (ideally up to 255 ) .Here to ease the platform setup, UEs are simulated via a single `lte-uesoftmodem` instance. Today the CI tests just with one UE and architecture has to be reviewed to allow a number of UE above about 16. This work is on-going.

As for the rf simulator, no specific hardware is required. The [L2 nfapi simlator page](L2NFAPI.md) contains the detailed documentation.

# L1 Simulator

The L1 simulator is using the ethernet fronthaul protocol, as used to connect a RRU and a RAU to connect UEs and a eNodeB. UEs are simulated in a single `lte-uesoftmodem` process, as for the nfapi simulator. 

The [L1 simulator page](L1SIM.md) contains the detailed documentation.

## noS1 mode

The noS1 mode is now available via the `--noS1`command line option. It can be used with simulators, described above, or when using oai with true RF boards. Only the oai UE can be connected to the oai eNodeB in noS1 mode.

By default the noS1 mode is using linux tun interfaces to send or receive ip packets to/from the linux ip stack. using the `--nokrnmod 0`option you can enforce kernel modules instead of tun.

noS1 code has been revisited, it has been tested with the rf simulator, and tun interfaces. More tests are on going and CI will soon include noS1 tests.

# Running with a true radio head

oai supports [number of deployment](FEATURE_SET.md) model, the following are tested in the CI:

1.  [Monolithic eNodeB](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/HowToConnectCOTSUEwithOAIeNBNew) where the whole signal processing is performed in a single process
2. if4p5 mode, where frequency domain samples are carried over ethernet, from the RRU which implement part of L1(FFT,IFFT,part of PRACH),  to a RAU

# 5G NR

As of February 2020, all 5G NR development is part of the develop branch (the branch develop-nr is no longer maintained). This also means that all new development will be merged into there once it passes all the CI. 

## NSA setup with COTS UE

This setup requires an EPC, an OAI eNB and gNB, and a COTS Phone. A dedicated page describe the setup can be found [here](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home/gNB-COTS-UE-testing).

### Launch gNB

```bash sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf```

### Launch eNB

```bash sudo ./lte-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.tm1.50PRB.usrpb210.conf```



## phy-test setup with OAI UE

The OAI UE can also be used in front of a OAI gNB without the support of eNB or EPC. In this case both gNB and eNB need to be run with the --phy-test flag. At the gNB this flag does the following
 - it reads the RRC configuration from the configuration file
 - it encodes the RRCConfiguration and the RBconfig message and stores them in the binary files rbconfig.raw and reconfig.raw
 - the MAC uses a pre-configured allocation of PDSCH and PUSCH with randomly generated payload

At the UE the --phy-test flag will
 - read the binary files rbconfig.raw and reconfig.raw from the current directory (a different directory can be specified with the flag --rrc_config_path) and process them.


### Launch gNB

```bash sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --phy-test```

### Launch UE in another window

```bash sudo ./nr-uesoftmodem --phy-test [--rrc_config_path ../../../ci-scripts/rrc-files]```

Some other useful paramters of the UE are

 - --ue-fo-compensation: enables the frequency offset compenstation at the UE. This is useful when running over the air and/or without an external clock/time source
 - --usrp-args: this is the equivalend paramter of sdr_addrs field in the gNB config file and can be used to identify the USRP and set some basic paramters (like the clock source)
 - --clock-source: sets the clock-source (internal or external). 
 - --time-source: sets the time-source (internal or external). 

## noS1 setup with OAI UE

Instead of randomly generated payload, in the phy-test mode we can also inject/receive user-plane traffic over a TUN interface. This is the so-called noS1 mode. 

This setup is described in the [rfsimulator page](../targets/ARCH/rfsimulator/README.md#5g-case). In theory this should also work with the real hardware target although this has yet to be tested.

## do-ra setup with OAI

The do-ra flag is used to ran the NR Random Access procedures in contention-free mode. Currently OAI implements the RACH process from Msg1 to Msg3. 

In order to run the RA, the following flag is needed for both the gNB and the UE:

`--do-ra`

### Run OAI in do-ra mode

From the `cmake_targets/ran_build/build` folder:

gNB on machine 1:

`sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --do-ra`

UE on machine 2:

`sudo ./nr-uesoftmodem --do-ra`

With the RF simulator (on the same machine):

`sudo RFSIMULATOR=gnb ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --do-ra --rfsim --parallel-config PARALLEL_SINGLE_THREAD`

`sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --do-ra --rfsim --parallel-config PARALLEL_SINGLE_THREAD`

## SA setup with OAI

The sa flag is used to run gNB in standalone mode.

In order to run gNB and UE in standalone mode, the following flag is needed:

`--sa`

At the gNB the --sa flag does the following:
- The RRC encodes SIB1 according to the configuration file and transmits it through NR-BCCH-DL-SCH.

At the UE the --sa flag will:
- Decode SIB1 and starts the 5G NR Initial Access Procedure for SA:
  1) 5G-NR RRC Connection Setup
  2) NAS Authentication and Security
  3) 5G-NR AS Security Procedure
  4) 5G-NR RRC Reconfiguration
  5) Start Downlink and Uplink Data Transfer

Command line parameters for UE in --sa mode:
- `C` : downlink carrier frequency in Hz (default value 0)
- `CO` : uplink frequency offset for FDD in Hz (default value 0)
- `numerology` : numerology index (default value 1)
- `r` : bandwidth in terms of RBs (default value 106)
- `band` : NR band number (default value 78)
- `s` : SSB start subcarrier (default value 512)

### Run OAI in SA mode

From the `cmake_targets/ran_build/build` folder:

gNB on machine 1:

`sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --sa`

UE on machine 2:

`sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --sa`

With the RF simulator (on the same machine):

`sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --rfsim --sa`

`sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa`

## IF setup with OAI

The `if_freq` and `if_freq_off` options can be used to set (via command line) custom downlink and uplink FR1 arbitrary frequencies for the IF equipment both.

In the same way, the following parameters must be configured in the RUs section of the gNB configuration file:

`if_freq`

`if_offset`

The values must be given in Hz.

### Run OAI with custom DL/UL arbitrary frequencies

The following example uses DL frequency 2169.080 MHz and UL frequency offset -400 MHz, with a configuration file for band 66 (FDD) at gNB side.

From the `cmake_targets/ran_build/build` folder:

gNB on machine 1:

`sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band66.tm1.106PRB.usrpx300.conf`

UE on machine 2:

`sudo ./nr-uesoftmodem --if_freq 2169080000 --if_freq_off -400000000`



[Selecting an alternative ldpc implementation at run time](../openair1/PHY/CODING/DOC/LDPCImplementation.md)

[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

