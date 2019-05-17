<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">Open Air LTE Emulation</font></b>
    </td>
  </tr>
</table>

This page is valid for the develop branch

# Table of Contents: #

* [How to build the eNB and the UE](#build)
* [How to run an eNB built with the noS1 option](#run-noS1-eNB)
* [How to run a UE built with the noS1 option](#run-noS1-UE)
* [Continuous Integration notes](#CInote)
* [How to ping an eNB from a UE and vice versa (with the noS1 option)](#noS1-pinging)

The old oaisim is dead! Long live oaisim! :)

If you are looking for a description of the old oaisim (which is still available in some branches/tags), please see [here](OpenAirLTEEmulation) and [here](how-to-run-oaisim-with-multiple-ue).

oaisim has been scraped and replaced by the same programs that are used for the real-time operation, `lte-softmodem` and `lte-uesoftmodem`. The latter also now includes an optional channel model, just like oaisim did. 

# <a name="build">[How to build the eNB and the UE](BUILD.md)</a>

The following paragraph explains how to run the L1 simulator in noS1 mode and using the oai kernel modules. 

# <a name="run-noS1-eNB">How to run an eNB with the noS1 option</a>

Modify the configuration file for IF4p5 fronthaul, `/openairinterface5g/ci-scripts/conf_files/rcc.band7.nos1.simulator.conf`, and replace the loopback interface with a physical ethernet interface and the IP addresses to work on your network. Copy your modifications to a new file, let's call YYY.conf the resulting configuration file.

Run lte-softmodem as usual with this configuration. 

```bash
$ source oaienv
$ cd cmake_targets/tools
$ sudo -E ./init_nas_nos1 eNB
$ cd ../lte_build_oai/build
$ sudo -E ./lte-softmodem -O YYY.conf --noS1 --nokrnmod 0
```

# <a name="run-noS1-UE">How to run a UE with the noS1 option</a>

Similarly modify the example configuration file in `/openairinterface5g/ci-scripts/conf_files/rru.band7.nos1.simulator.conf` and replace loopback interface and IP addresses. Copy your modifications to a new file, let's call XXX.conf the resulting configuration file.

Run it like:

```bash
$ source oaienv
$ cd cmake_targets/tools
$ sudo -E ./init_nas_nos1 UE
$ cd ../lte_build_oai/build
$ sudo ./lte-uesoftmodem -O XXX.conf -r 25 --siml1 --noS1 --nokrnmod 0
```

That should give you equivalent functionality to what you had with oaisim including noise and RF channel emulation (path loss / fading, etc.). You should also be able to run multiple UEs. 

# <a name="CInote">Continuous Integration notes</a>
The CI currently tests the noS1 build option with one eNB and one UE in the following scenarios:
* pinging one UE from one eNB
* pinging one eNB from one UE
* iperf download between one eNB and one UE
* iperf upload between one eNB and one UE
* all the above tests are done in FDD 5Mhz mode.

# <a name="noS1-pinging">How to ping an eNB from a UE and vice versa (with the noS1 option)</a>

Once your eNB and UE (built with the noS1 option) are running and synchronised, you can ping the eNB from the UE with the following command:

```bash
ping -I oai0 -c 20 $eNB_ip_addr
```
where $eNB_ip_addr is the IP address of your eNB.

Similarly, you can ping the UE from the eNB.

The IP adresses of the eNB and the UE are set up by the init_nas_nos1 program and should have the following values:
* eNB_ip_addr set to 20 10.0.1.1
* ue_ip_addr set to 20 10.0.1.2







[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

