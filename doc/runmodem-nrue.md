<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# General information for nrUE and configuration

This page describes the configuration of the nrUE (applicable for use with any
gNB) as well as OAI-specific modes.

[[_TOC_]]

## General configuration

Command line parameters for UE in standalone mode:
- `-C` : downlink carrier frequency in Hz (default value 0)
- `--CO` : uplink frequency offset for FDD in Hz (default value 0)
- `--numerology` : numerology index (default value 1)
- `-r` : bandwidth in terms of RBs (default value 106)
- `--band` : NR band number (default value 78)
- `--ssb` : SSB start subcarrier (default value 516)

To simplify the configuration for the user testing OAI UE with OAI gNB, the latter prints the following LOG that guides the user to correctly set some of the UE command line parameters:

```shell
[PHY]   Command line parameters for OAI UE: -C 3319680000 -r 106 --numerology 1 --ssb 516
```

You can run this, using USRPs, on two separate machines:

```shell
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --gNBs.[0].min_rxtxtime 6
sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516
```

With the **RFsimulator** (on the same machine), just add the option `--rfsim` to both gNB and NR UE command lines.

UE capabilities can be passed according to the [UE Capabilities](#ue-capabilities) section.

## Configuration file

The UE can be provided a configuration file on the command line using `-O
<config>`. At the very least, the configuration file should always contain
information on IMSI, key, and PDU session to use for the information:

```shell
uicc0 = {
  imsi = "001010000000001";
  key = "fec86ba6eb707ed08905757b1bb44b8f";
  opc = "C42449363BBAD02B66D16BC975D77CC1";
  pdu_sessions = ({ dnn = "oai"; nssai_sst = 1; });
}
```

| **Parameter** | **Description** | **Default Value** |
|---------------|-----------------|-------------------|
| **IMSI** | Unique identifier for the UE within the mobile network. Used by the network to identify the UE during authentication. It ensures that the UE is correctly identified by the network. | 001010000000001 |
| **key** | Cryptographic key shared between the UE and the network, used for encryption during the authentication process. | `fec86ba6eb707ed08905757b1bb44b8f` |
| **OPC** | Operator key for the Milenage Authentication and Key Agreement algorithm used for encryption during the authentication process. | Ensures secure communication between the UE and the network by matching the encryption keys. | `C42449363BBAD02B66D16BC975D77CC1` |
| **pdu_sessions** | list of PDU sessions to request | empty array (no PDU session) |

Note that the legacy DNN and NSSAI parameters are not supported anymore, and
`pdu_sessions` should be defined in the config style, or the UE will not
request a PDU session.

Each element within the `pdu_sessions` array takes the following parameters.
Multiple PDU sessions can be requested.

| **Parameter** | **Description** | **Default Value** |
|---------------|-----------------|-------------------|
| `id`          | ID of the PDU session to request | index of the current element (1..16) |
| `type`        | Type of the PDU session to request (allowed: `IPv4`, `IPv6`, `IPv4v6`, `Ethernet` | `IPv4` |
| `dnn`         | Specifies the name of the data network the UE wishes to connect to | `oai` |
| `nssai_sst`   | Slice Service Type to request (1=eMBB, 2=URLLC, 3=mMTC) | `1` |
| `nssai_sd`   | Slice Differentiator to request | `0xffffff` (meaning "no SD") |
| `ifname`      | Interface name to be used for this PDU session | auto-generated name |

For instance, to request two PDU sessions with user-defined IDs, you could
use.

```
uicc0 = {
  # ...
  pdu_sessions = (
    { id=1; dnn = "oai.ipv4"; type = "IPv4", nssai_sst = 1; },
    { id=2; dnn = "oai.ipv6"; type = "IPv6", nssai_sst = 1; ifname = "ue.ipv6"; },
  );
}
```

For the second PDU session, an interface name to be used has been requested.
By default (if not provided), the interface names follow the scheme
`oaitun_ueX[_pY]` and `oaitap_ueX[_pY]` where

-  `oaitun` is used for IP PDU sessions (TUN), and `oaitap` is used for
   Ethernet PDU sessions (TAP)
- `ueX` with `X` referring to the UE index, starting at 1, if multiple UEs run
  in a single process
- `pY` is an optional suffix that is appended for each 2nd, 3rd, ..., PDU session.

An explicit interface name can be requested to allow these use cases:

1. running multiple UE processes, which by default would all use `oaitun_ue1`,
   i.e., to avoid conflicts
2. where the existing names are not descriptive enough or need to fit into
   existing workflows.
3. to bind to a pre-existing, named interface. To do this, create a new
   interface before running the UE, then configure the IP address manually that
   is received by the UE. Example:
   ```
   sudo ip tuntap add name ue_wan mode tun
   ./nr-uesoftmodem ... --uicc0.pdu_sessions.[0].ifname ue_wan
   sudo ip addr add <IP>/24 dev ue_wan # replace with IP shown in logs
   sudo ip link set ue_wan up
   sudo ip tuntap delete name ue_wan mode tun # remove once not needed
   ```
   In this case, in RFsim, the UE does not need root privileges anymore

### Optional NR-UE command line options

Here are some useful command line options for the NR UE:

| Parameter                | Description                                                                                                   |
|--------------------------|---------------------------------------------------------------------------------------------------------------|
| `--ue-scan-carrier`      | Scan for cells in current bandwidth. This option can be used if the SSB position of the gNB is unknown. If multiple cells are detected, the UE will try to connect to the first cell. By default, this option is disabled and the UE attempts to only decode SSB given by `--ssb`. |
| `--ue-fo-compensation`   | Enables the initial frequency offset compensation at the UE. Useful when running over the air and/or without an external clock/time source. |
| `--cont-fo-comp`         | Enables the continuous frequency offset (FO) estimation and compensation.  Parameter value `1` specifies that the main FO contribution comes from the local oscillator's (LO) accuracy.  Parameter value `2` specifies that the main FO contribution comes from Doppler shift. Parameter value `3` specifies that no measured residual DL FO is considered for UL FO pre-compensation. |
| `--initial-fo`           | Sets the known initial frequency offset. Useful especially with large Doppler frequency, e.g. LEO satellite.  |
| `--freq-sync-P`          | Sets the coefficient for the Proportional part of the PI-controller for the continuous frequency offset compensation. Default value 0.01. |
| `--freq-sync-I`          | Sets the coefficient for the Integrating part of the PI-controller for the continuous frequency offset compensation. Default value 0.001. |
| `--num-ues`              | Run multiple UEs in one process                                                                               |
| `--ntn-initial-time-drift` | Sets the initial NTN DL time drift (feeder link and service link), given in µs/s.                           |
| `--autonomous-ta`        | Enables the autonomous TA update, based on DL drift (useful if main contribution to DL drift is movement, e.g. LEO satellite). |
| `--time-sync-P`          | Sets the coefficient for the Proportional part of the PI-controller for the time synchronization. Default value 0.5. |
| `--time-sync-I`          | Sets the coefficient for the Integrating part of the PI-controller for the time synchronization. Default value 0.0. |
| `--disable-blind-search` | Disables the blind search for neighboring cells. Useful for less powerful computers. |
| `--usrp-args`            | Equivalent to the `sdr_addrs` field in the gNB config file. Used to identify the USRP and set some basic parameters (like the clock source).  |
| `--clock-source`         | Sets the clock source (internal or external).                                                                 |
| `--time-source`          | Sets the time source (internal or external).                                                                  |
| `--ue-capability-rx2tx`  | Sets the minimum number of slots for the UE between RX command and TX of corresponding channel.               |

You can view all available options by typing:

```shell
./nr-uesoftmodem --help
```

### UE Capabilities

The `--uecap_file` option can be used to pass the UE Capabilities input file
(path location + filename):

- `targets/PROJECTS/GENERIC-NR-5GC/CONF/uecap_portsX.xml` for X=1,2,4 layer(s).
  Those are sample files and might need to be tweaked for your scenario.

Not providing this option yields empty UE capabilities being sent to the gNB.

This option is available for the following combinations of operation modes and gNB/nrUE softmodems:

| Mode       | Executable     | Description                                         |
|------------|----------------|-----------------------------------------------------|
| SA         | nr-uesoftmodem | Send UE capabilities from the UE to the gNB via RRC |
| phy-test   | nr-softmodem   | Mimic the reception of UE capabilities by the gNB   |
| do-ra      | nr-softmodem   | Mimic the reception of UE capabilities by the gNB   |

e.g.

```shell
sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3319680000 --ue-nb-ant-tx 2 --ue-nb-ant-rx 2 --uecap_file /opt/oai-nr-ue/etc/uecap.xml
```

## NR UE: Configure multiple RF-frontends (RUs) and UEs in one process

Multiple RF-frontends (also called RUs) can be defined for the nr-uesoftmodem.
Therefore, two sections in the NR UE configuration file are used:
- `RUs`
- `cells`

The `RUs` section in the NR UE configuration file contains an array of elements, where each element has these properies:

| Property name    | Type           | Default value | Description                          |
| ---------------- | -------------- | ------------- | ------------------------------------ |
| nb_tx            | integer        | 1             | Number of TX Antennas                |
| nb_rx            | integer        | 1             | Number of RX Antennas                |
| att_tx           | integer        | 0             | TX Attenuation in dB                 |
| att_rx           | integer        | 0             | RX Attenuation in dB                 |
| max_rxgain       | integer        | 120           | Maximum RX Gain at 0 dB Attenuation  |
| sdr_addrs        | string         | type=b200     | SDR Parameter String                 |
| tx_subdev        | string         |               | SDR TX Subdevice                     |
| rx_subdev        | string         |               | SDR RX Subdevice                     |
| clock_src        | string         | internal      | SDR Clock Source                     |
| time_src         | string         | internal      | SDR Time Source                      |
| tune_offset      | floating point | 0.0           | SDR Tune Offset in Hz                |
| if_freq          | integer        | 0             | DL Intermediate Frequency in Hz      |
| if_offset        | integer        | 0             | UL Intermediate Frequency Offset in Hz |

The `cells` section in the NR UE configuration file contains an array of elements, where each element has these properies:

| Property name    | Type    | Default value | Description                              |
| ---------------- | ------- | ------------- | ---------------------------------------- |
| ru_id            | integer | 0             | ID of the associated RU from the `RUs` section |
| band             | integer | 78            | 5G NR Band                               |
| rf_freq          | integer | 0             | DL Carrier Centre Frequency in Hz        |
| rf_offset        | integer | 0             | DL Carrier Centre Frequency Offset in Hz |
| numerology       | integer | 1             | 5G NR Numerology (µ)                     |
| N_RB_DL          | integer | 106           | Number of DL Carrier Ressource Blocks    |
| ssb_start        | integer | 516           | Ressource Element where the SSB Starts   |

There are different scenarios where multiple RF-frontends (also called RUs) are beneficial for the NR UE:

1. RF-Simulator Inter-Frequency Handover between multiple cells
2. Multiple UEs in one instance, each using their own RF-frontend (RF-Simulator connection)
3. Different Antennas connected to different RF-ports
4. Concurrent connection to multiple carriers (carrier aggregation CA)

This would be and example configuration for the 1. scenario:

```
rfsimulator = (
    {
        serveraddr = "127.0.0.2";
        serverport = 4043;
    }, {
        serveraddr = "127.0.0.3";
        serverport = 4044;
    }
);

RUs = (
    {
        nb_tx = 1;
        nb_rx = 1;
    }, {
        nb_tx = 1;
        nb_rx = 1;
    }
);

cells = (
    {
        ru_id      = 0;
        band       = 78;
        rf_freq    = 3619200000L;
        numerology = 1;
        N_RB_DL    = 106;
        ssb_start  = 516;
    }, {
        ru_id      = 1;
        band       = 78;
        rf_freq    = 3649440000L;
        numerology = 1;
        N_RB_DL    = 106;
        ssb_start  = 516;
    }
);
```

An example for the 2. scenario can be found in the file
[ci-scripts/yaml_files/5g_rfsimulator_multiue/nrue.uicc.conf](../ci-scripts/yaml_files/5g_rfsimulator_multiue/nrue.uicc.conf).
Multiple UEs run in one process, and since no RU sharing is implemented (see
below), each UE runs its own RF device, e.g., an RFsimulator connection.

The number of UE instances is controlled by `--num-ues N`. This creates N
independent UE protocol stacks (PHY, MAC, NAS). UE instance `i` reads its
credentials from the `uicc{i}` config section and is assigned `cells[i]` in
order. The config must therefore have at least N `uicc`, `cells`, and `RUs`
entries. Note that **all RUs defined in the `RUs` section are opened and
initialized regardless of `--num-ues`**; extra entries consume resources
without being used by any UE instance.

The 3. scenario is similar to 1., but instead of providing RF-Simulator parameters, actual SDR parameters have to be provided.

The 4. scenario is not supported, as the NR UE does not implement CA, yet.

Current Limitations:
- Each RU can be used by only one cell.
- Each RU and cell can be used by only one UE (no RU sharing implemented, yet).
- The sampling rates of all RUs must be the same.

## Specific OAI modes

These modes are applicable when running both OAI UE and OAI gNB together.

### phy-test setup with OAI UE

The OAI UE can also be used in front of a OAI gNB without the support of eNB or EPC and circumventing random access. In this case both gNB and eNB need to be run with the `--phy-test` flag. At the gNB this flag does the following
 - it reads the RRC configuration from the configuration file
 - it encodes the RRCConfiguration and the RBconfig message and stores them in the binary files `rbconfig.raw` and `reconfig.raw` in the current directory
 - the MAC uses a pre-configured allocation of PDSCH and PUSCH with randomly generated payload instead of the standard scheduler. The options `-m`, `-l`, `-t`, `-M`, `-T`, `-D`, and `-U` can be used to configure this scheduler.
 - Options `-Dmod`, and `-Umod` were introduced to enable scheduling PDSCH/PUSCH on slots >= 64 in phy-test mode. (in case of >= 120Khz subcarrier spacing and FDD)
 - For ex: `-Dmod 2' / '-Umod 2` allocates every 2nd slot for PDSCH or PUSCH respectively.
 - See `./nr-softmodem -h` for more information.

At the UE, the `--phy-test` flag will read the binary files `rbconfig.raw` and `reconfig.raw` from the current directory and process them. If you wish to provide a different path for these files, please use the options `--reconfig-file` and `--rbconfig-file`.

```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --phy-test
```

```bash
sudo ./nr-uesoftmodem --phy-test [--reconfig-file ../../../ci-scripts/rrc-files/reconfig.raw --rbconfig-file ../../../ci-scripts/rrc-files/rbconfig.raw]
```

In summary:
- If you are running on the same machine and launched the 2 executables (`nr-softmodem` and `nr-uesoftmodem`) from the same directory, nothing has to be done.
- If you launched the 2 executables from 2 different folders, just point to the location where you launched the `nr-softmodem`:
  * `sudo ./nr-uesoftmodem --rfsim --phy-test --reconfig-file /the/path/where/you/launched/nr-softmodem/reconfig-file --rbconfig-file /the/path/where/you/launched/nr-softmodem/rbconfig-file --rfsimulator.[0].serveraddr <TARGET_GNB_INTERFACE_ADDRESS>`
- If you are not running on the same machine, you need to **COPY** the two raw files
  * `scp usera@machineA:/the/path/where/you/launched/nr-softmodem/r*config.raw userb@machineB:/the/path/where/you/will/launch/nr-uesoftmodem/`
  * Obviously this operation should be done before launching the `nr-uesoftmodem` executable.

In phy-test mode it is possible to mimic the reception of UE Capabilities at gNB through the command line parameter `--uecap_file`. Refer to the [UE Capabilities](#ue-capabilities) section for more details.

### noS1 setup with OAI UE

Instead of randomly generated payload, in the phy-test mode we can also
inject/receive user-plane traffic over a TUN interface. This is the so-called
noS1 mode.

The noS1 mode is applicable to both gNB/UE, and enabled by passing `--noS1` as
an option. The gNB/UE will open a TUN interface which the interface names and
IP addresses `oaitun_enb1`/10.0.1.1, and `oaitun_ue1`/10.0.1.2, respectively.
You can then use these interfaces to send traffic, e.g.,
```bash
iperf -sui1 -B 10.0.1.2
```
to open an iperf server on the UE side, and
```bash
iperf -uc 10.0.1.2 -B 10.0.1.1 -i1 -t10 -b1M
```
to send data from the gNB down to the UE.

> Note that this does not work if both interfaces are on the same host. We
recommend to use two different hosts, or at least network namespaces, to route
traffic through the gNB/UE tunnel.

This option is only really helpful for phy-test/do-ra (see below) modes, in
which the UE does not connect to a core network. If the UE connects to a core
network, it receives an IP address for which it automatically opens a network
interface.

### do-ra setup with OAI

The do-ra flag is used to ran the NR Random Access procedures in contention-free mode. Currently OAI implements the RACH process from Msg1 to Msg3. 

In order to run the RA, the `--do-ra` flag is needed for both the gNB and the UE.

In do-ra mode it is possible to mimic the reception of UE Capabilities at gNB through the command line parameter `--uecap_file`. Refer to the [UE Capabilities](#ue-capabilities) section for more details.

To run using the RFsimulator:

```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --do-ra --rfsim
sudo ./nr-uesoftmodem --do-ra --rfsim --rfsimulator.[0].serveraddr 127.0.0.1
```

Using USRPs:

```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --do-ra
```

On a separate machine:

```bash
sudo ./nr-uesoftmodem --do-ra
```
