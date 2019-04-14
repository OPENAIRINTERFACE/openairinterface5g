<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">L2 nFAPI Simulator (with S1 / same machine deployment)</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.   [Environment](#1-environment)
2.   [Prepare the EPC](#2-prepare-the-epc)
3.   [Retrieve the OAI eNB-UE source code](#3-retrieve-the-oai-enb-ue-source-code)
4.   [Setup of the USIM information in UE folder](#4-setup-of-the-usim-information-in-ue-folder)
5.   [Setup of the Configuration files](#5-setup-of-the-configuration-files)
     1.   [The eNB Configuration file](#51-the-enb-configuration-file)
     2.   [The UE Configuration file](#52-the-ue-configuration-file)
6.   [Bring Up a second loopback interface](#6-bring-up-a-second-loopback-interface)
7.   [Build the eNB](#7-build-the-enb)
8.   [Build the UE](#8-build-the-ue)
9.   [Initialize the NAS UE Layer](#9-initialize-the-nas-ue-layer)
10.   [Start the eNB](#10-start-the-enb)
11.   [Start the UE](#11-start-the-ue)
12.   [Test with ping](#12-test-with-ping)
13.   [Limitations](#13-limitations)

# 1. Environment #

2 servers are used in this deployment. You can use Virtual Machines instead of each server; like it is done in the CI process.

*  Machine A contains the EPC.
*  Machine B contains the OAI eNB and the OAI UE(s)

Example of L2 nFAPI Simulator testing environment:

<img src="../l2-nfapi-simulator/L2-sim-single-server-deployment.png" alt="" border=3>

Note that the IP addresses are indicative and need to be adapted to your environment.

# 2. Prepare the EPC #

Create the environment for the EPC and register all **USIM** information into the **HSS** database.

If you are using OAI-EPC ([see on GitHub](https://github.com/OPENAIRINTERFACE/openair-cn)), build **HSS/MME/SPGW** and create config files.

# 3. Retrieve the OAI eNB-UE source code #

The eNB and the UE executables will compiled into 2 separate folders on the same machine `B`.

```bash
$ ssh sudousername@machineB
$ git clone https://gitlab.eurecom.fr/oai/openairinterface5g/ enb_folder
$ cd enb_folder
$ git checkout -f v1.0.0
$ cd ..
$ cp -Rf enb_folder ue_folder
```

# 4. Setup of the USIM information in UE folder #

```bash
$ ssh sudousername@machineB
$ cd ue_folder
# Edit openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf with your preferred editor
```

Edit the USIM information within this file in order to match the HSS database. They **HAVE TO** match:

*  PLMN+MSIN and IMSI of users table of HSS database **SHALL** be the same.
*  OPC of this file and OPC of users table of HSS database **SHALL** be the same.
*  USIM_API_K of this file and the key of users table of HSS database **SHALL** be the same.

When testing multiple UEs, it is necessary to add other UEs information like described below for 2 Users. Only UE0 (first UE) information is written in the original file.

```
UE0:
{
    USER: {
        IMEI="356113022094149";
        MANUFACTURER="EURECOM";
        MODEL="LTE Android PC";
        PIN="0000";
    };

    SIM: {
        MSIN="0000000001";  // <-- Modify here
        USIM_API_K="8baf473f2f8fd09487cccbd7097c6862";
        OPC="e734f8734007d6c5ce7a0508809e7e9c";
        MSISDN="33611123456";
    };
...
};
// Copy the UE0 and edit
UE1: // <- Edit here
{
    USER: {
        IMEI="356113022094149";
        MANUFACTURER="EURECOM";
        MODEL="LTE Android PC";
        PIN="0000";
    };

    SIM: {
        MSIN="0000000002";  // <-- Modify here
        USIM_API_K="8baf473f2f8fd09487cccbd7097c6862";
        OPC="e734f8734007d6c5ce7a0508809e7e9c";
        MSISDN="33611123456";
    };
...
};
```

You can repeat the operation for as many users you want to test with.

# 5. Setup of the Configuration files #

**CAUTION: both proposed configuration files resides in the ci-scripts realm. You can copy them but you CANNOT push any modification on these 2 files as part of an MR without informing the CI team.**

## 5.1. The eNB Configuration file ##

```bash
$ ssh sudousername@machineB
$ cd enb_folder
# Edit ci-scripts/conf_files/rcc.band7.tm1.nfapi.conf with your preferred editor
```

First verify the nFAPI interface setup on the 2nd loopback interface.

```
MACRLCs = (
        {
        num_cc = 1;
        local_s_if_name  = "lo:";            // <-- HERE
        remote_s_address = "127.0.0.1";      // <-- HERE
        local_s_address  = "127.0.0.2";      // <-- HERE
        local_s_portc    = 50001;
        remote_s_portc   = 50000;
        local_s_portd    = 50011;
        remote_s_portd   = 50010;
        tr_s_preference = "nfapi";
        tr_n_preference = "local_RRC";
        }
);
```

If you are testing more than 16 UEs, a proper setting on the RUs is necessary. **Note that this part is NOT present in the original configuration file**.

```
RUs = (
    {
       local_rf       = "yes"
         nb_tx          = 1
         nb_rx          = 1
         att_tx         = 20
         att_rx         = 0;
         bands          = [38];
         max_pdschReferenceSignalPower = -23;
         max_rxgain                    = 116;
         eNB_instances  = [0];
    }
);
```

Last, the S1 interface shall be properly set.

```
    ////////// MME parameters:
    mme_ip_address      = ( { ipv4       = "CI_MME_IP_ADDR"; // replace with 192.168.10.20
                              ipv6       = "192:168:30::17";
                              active     = "yes";
                              preference = "ipv4";
                            }
                          );

    NETWORK_INTERFACES :
    {
        ENB_INTERFACE_NAME_FOR_S1_MME            = "ens3";            // replace with the proper interface name
        ENB_IPV4_ADDRESS_FOR_S1_MME              = "CI_ENB_IP_ADDR";  // replace with 192.168.10.10
        ENB_INTERFACE_NAME_FOR_S1U               = "ens3";            // replace with the proper interface name
        ENB_IPV4_ADDRESS_FOR_S1U                 = "CI_ENB_IP_ADDR";  // replace with 192.168.10.10
        ENB_PORT_FOR_S1U                         = 2152; # Spec 2152
        ENB_IPV4_ADDRESS_FOR_X2C                 = "CI_ENB_IP_ADDR";  // replace with 192.168.10.10
        ENB_PORT_FOR_X2C                         = 36422; # Spec 36422

    };
```

## 5.2. The UE Configuration file ##

```bash
$ ssh sudousername@machineB
$ cd ue_folder
# Edit ci-scripts/conf_files/ue.nfapi.conf with your preferred editor
```

Verify the nFAPI interface setup on the loopback interface.

```
L1s = (
        {
        num_cc = 1;
        tr_n_preference = "nfapi";
        local_n_if_name  = "lo";         // <- HERE
        remote_n_address = "127.0.0.2";  // <- HERE
        local_n_address  = "127.0.0.1";  // <- HERE
        local_n_portc    = 50000;
        remote_n_portc   = 50001;
        local_n_portd    = 50010;
        remote_n_portd   = 50011;
        }
);
```

# 6. Bring Up a second loopback interface #

A second loopback interface is used to connect the eNB and the UEs.

```bash
$ ssh sudousername@machineB
$ sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up
```

# 7. [Build OAI UE and eNodeB](BUILD.md) #



# 8. Initialize the NAS UE Layer #

Start the EPC on machine `A`.

```bash
$ ssh sudousername@machineA
# Start the EPC
```

# 9. Start the eNB #

In the first terminal (the one you used to build the eNB):

```bash
$ ssh sudousername@machineB
$ cd enb_folder/cmake_targets
$ sudo -E ./lte_build_oai/build/lte-softmodem -O ../ci-scripts/conf_files/rcc.band7.tm1.nfapi.conf > enb.log 2>&1
```

If you don't use redirection, you can test but many logs are printed on the console and this may affect performance of the L2-nFAPI simulator.

We do recommend the redirection in steady mode once your setup is correct.

# 10. Start the UE #

In the second terminal (the one you used to build the UE):

```bash
$ ssh sudousername@machineB
$ cd ue_folder/cmake_targets
# Test 64 UEs, 64 threads in FDD mode
$ sudo -E ./lte_build_oai/build/lte-uesoftmodem -O ../ci-scripts/conf_files/ue.nfapi.conf --L2-emul 3 --num-ues 64 --nums-ue-thread 64 > ue.log 2>&1
# Test 64 UEs, 64 threads in TDD mode
$ sudo -E ./lte_build_oai/build/lte-uesoftmodem -O ../ci-scripts/conf_files/ue.nfapi.conf --L2-emul 3 --num-ues 64 --nums-ue-thread 64 -T 1 > ue.log 2>&1
# The "-T 1" option means TDD config
```

-   The number of UEs can set by using `--num-ues` option and the maximum UE number is 255 (with the `--mu*` options, otherwise 16).
-   The umber of threads can set with the `--nums-ue-thread`. This number **SHALL NOT** be greater than the number of UEs.
-   How many UE that can be tested depends on hardware (server , PC, etc) performance in your environment.

# 11. Test with ping #

In a third terminal, after around 10 seconds, the UE(s) shall be connected to the eNB:

```bash
$ ssh sudousername@machineA
# Ping UE0 IP address based on the EPC pool used: in this example:
$ ping -c 20 192.168.200.2
# Ping UE1 IP address based on the EPC pool used: in this example:
$ ping -c 20 192.168.200.4
```

# 12. Limitations #

Testing on the CI process is currently limited at time of writing. Improvements will be made soon.









[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

[L2 nfapi simulator](L2NFAPI.md)
