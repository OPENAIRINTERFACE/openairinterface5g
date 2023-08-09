<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">L2 nFAPI Simulator Usage</font></b>
    </td>
  </tr>
</table>

## 4G L2 nFAPI simulator

This simulator allows to test L2 and above Layers using the nFAPI interface.

The UE executable is able to "simulate" multiple UEs in order to stimulate the scheduler in the eNB.

**This simulator is available starting the `v1.0.0` release on the `master` branch.**

**2022/03/08: CAUTION, THIS TUTORIAL IS NO LONGER VALID on the `develop` branch after the `2022.w01` tag.**

**2022/03/08: CAUTION, THE LAST VALID TAG on `develop` branch is `2021.w51_c`.**

Currently the Continuous Integration process is validating this simulator the following way:

*  the LTE modem executable is run on one host (in our CI deployment it is a **Xenial Virtual Machine**)
*  the UE(s) modem executable is run on another host (in our CI deployment it is also a **Xenial Virtual Machine**)
*  We are testing:
   *   in S1 mode (ie we are connected to a 3rd-party EPC)
   *   in noS1 mode (no need for an EPC)

Normally it should be fine to run both executables on the same host using the `loopback` interface to communicate. **But we are not guaranting it**

1. [With S1 -- eNB and UE on 2 hosts](L2NFAPI_S1.md)
2. [No S1 -- eNB and UE on 2 hosts](L2NFAPI_NOS1.md)


**2022/03/08: Starting the `2022.w01` tag on the `develop` branch, the L2 nFAPI simulation is using a proxy.**

## 5G L2 nFAPI simulator

### Download and Build the Proxy Server (from EpiSci)

```bash
$ cd ~
$ git clone https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy.git
$ cd oai-lte-multi-ue-proxy
$ make
```

### Start Proxy Server
Set the number of UEs to connect to server as 1 and the mode is SA.
```bash
$ number_of_ues=1
$ sudo -E ./build/proxy $number_of_ues --nr
```

### Start gNB
```bash
$ sudo -E ./nr-softmodem -O ../ci-scripts/conf_files/episci/proxy_rcc.band78.tm1.106PRB.nfapi.conf --nfapi VNF --noS1 --sa --emulate-l1
```
### Start UE
```bash
$ sudo -E ./nr-uesoftmodem -O ../ci-scripts/conf_files/nrue.uicc.conf --nfapi STANDALONE_PNF --node-number 2 --sa --emulate-l1
```
### EpiSci's Documentation
A more detailed tutorial is available on the [EpiSci GitHub Repository](https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy#readme).

This proxy allows to perform L2 nFAPI simulator for:

* LTE
* 5G-NSA
* 5G-SA

### L2 Simulator as Docker Containers
Another tutorial for 5G SA mode with 1 User is available [here](../ci-scripts/yaml_files/5g_l2sim_tdd/README.md).

----

[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[oai softmodem build procedure](BUILD.md)

