[[_TOC_]]

# General

This is an RF simulator that allows to test OAI without an RF board. It
replaces an actual RF board driver. In other words, towards the xNB/UE, it
behaves like a "real" RF board, but it forwards samples between both ends
instead of sending it over the air. It can simulate simple channels, such as
AWGN, hence it *simulates* RF.

As much as possible, it works like an RF board, but not in real-time: It can
run faster than real-time if there is enough CPU, or slower (it is CPU-bound
instead of real-time RF sampling-bound).

# Build

## From [build_oai](../../../doc/BUILD.md) script
The RF simulator is implemented as an OAI device and always built when you build the OAI eNB or the OAI UE.

Using the `-w SIMU` option it is possible to just re-build the RF simulator device.

Example:
```bash
./build_oai --UE --eNB --gNB --nrUE --ninja -w SIMU
```

## Add the rfsimulator after initial build

After any regular build you can compile the device from the build directory:
```bash
cd <path to oai sources>/openairinterface5g/cmake_targets/ran_build/build
ninja rfsimulator
```

This is equivalent to using `-w SIMU` when running the `build_oai` script.

# Usage

## Overview

To use the RF simulator add the `--rfsim` option to the command line. By
default the RF simulator device will try to connect to host 127.0.0.1, port
4043, which is usually the behavior for the UE.  For the eNB/gNB, you either
have to pass `--rfsimulator.serveraddr server` on the command line, or specify
the corresponding section in the configuration file.

The RF simulator is using the configuration module, and its parameters are defined in a specific section called "rfsimulator".

| parameter            | usage                                                                                                             | default |
|:---------------------|:------------------------------------------------------------------------------------------------------------------|----:|
| serveraddr           | ip address to connect to, or `server` to behave as a tcp server                                                      | 127.0.0.1 |
| serverport           | port number to connect to or to listen on (eNB, which behaves as a tcp server)                                    | 4043 |
| options              | list of comma separated run-time options, two are supported: `chanmod` to enable channel modeling and `saviq` to write transmitted iqs to a file | all options disabled  |
| modelname            | Name of the channel model to apply on received iqs when the `chanmod` option is enabled                           | AWGN |
| IQfile               | Path to the file to be used to store iqs, when the `saviq` option is enabled                                      | /tmp/rfsimulator.iqs |

## How to use the RF simulator options

To define and use a channel model, the configuration file needs to include a
channel configuration file. To do this, add `@include "channelmod_rfsimu.conf"`
to the end of the configuration file, and place the channel configuration file
in the same directory. An example channel configuration file is
[`ci-scripts/conf_files/channelmod_rfsimu.conf`](../../ci-scripts/conf_files/channelmod_rfsimu.conf).

Add the following options to the command line to enable the channel model and the IQ samples saving for future replay:
```bash
--rfsimulator.options chanmod,saviq
```
or just:
```bash
--rfsimulator.options chanmod
```
to enable the channel model.

set the model with:
```bash
--rfsimulator.modelname AWGN
```

Example run:

```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --parallel-config PARALLEL_SINGLE_THREAD --rfsim --phy-test --rfsimulator.options chanmod --rfsimulator.modelname AWGN
```

where `@include "channelmod_rfsimu.conf"` has been added at the end of the file, and `ci-scripts/conf_files/channelmod_rfsimu.conf` copied to `targets/PROJECTS/GENERIC-LTE-EPC/CONF/`.

## 4G case

For the eNB, use a valid configuration file setup for the USRP board tests and start the softmodem with the `--rfsim` and `--rfsimulator.serveraddr server` options.
```bash
sudo ./lte-softmodem -O <config file> --rfsim --rfsimulator.serveraddr server
```
Often, configuration files define the corresponding `rfsimulator` section, in
which case you might omit `--rfsimulator.serveraddr server`. Example:
```
rfsimulator : {
  serveraddr = "server";
};
```

For the UE, it should be set to the IP address of the eNB. For instance, if the
eNB runs on another host with IP `192.168.2.200`, do
```bash
sudo ./lte-uesoftmodem -C 2685000000 -r 50 --rfsim --rfsimulator.serveraddr 192.168.2.200
```
For running on the same host, only `--rfsim` is necessary.

The UE and the eNB can be used as if the RF is real. The noS1 mode might be used as well with the RF simulator.

If you reach 'RA not active' on UE, be careful to generate a valid SIM.
```bash
$OPENAIR_DIR/cmake_targets/ran_build/build/conf2uedata -c $OPENAIR_DIR/openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o .
```

## 5G case

Similarly as for 4G, first launch the gNB, here in an example for the phytest:

```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --gNBs.[0].min_rxtxtime 6 --phy-test --rfsim --rfsimulator.serveraddr server
```

`--gNBs.[0].min_rxtxtime 6` is due to the UE not being able to handle shorter
RX/TX times.  As in the 4G case above, you can define an `rfsimulator` section
in the config file.

Then, launch the UE:

```bash
sudo ./nr-uesoftmodem --rfsim --phy-test --rfsimulator.serveraddr <TARGET_GNB_IP_ADDRESS>
```

Notes:

1. This starts the gNB and UE in the `phy-test` UP-only mode where the gNB is started as if a UE had already connected. See [RUNMODEM.md](../../doc/RUNMODEM.md) for more details.
2. `<TARGET_GNB_IP_ADDRESS>` should be the IP interface address of the remote host running the gNB executable, if the gNB and nrUE run on separate hosts, or be omitted if they are on the same host.
3. To enable the noS1 mode, `--noS1` option should be added to the command line, see again [RUNMODEM.md](../../doc/RUNMODEM.md).
4. To operate the gNB/UE with a 5GC, start them using the `--sa` option. More information can be found [here](../../../doc/NR_SA_Tutorial_OAI_CN5G.md).

## Store and replay

You can store emitted I/Q samples. If you set the option `saviq`, the simulator will write all the I/Q samples into this file. Then, you can replay with the executable `replay_node`.

First compile it like other binaries:
```bash
make replay_node
```
You can use this binary as I/Q data source to feed whatever UE or gNB with recorded I/Q samples.

The file format is successive blocks of a header followed by the I/Q array. If you have existing stored I/Q, you can adapt the tool `replay_node` to convert your format to the rfsimulator format.

The format intends to be compatible with the OAI store/replay feature on USRP.

## Channel simulation

When the `chanmod` option is enabled, the RF channel simulator is called.

In the current version all channel parameters are set depending on the model name via a call to:
```bash
new_channel_desc_scm(bridge->tx_num_channels,
                     bridge->rx_num_channels,
                     <model name>,
                     bridge->sample_rate,
                     bridge->tx_bw,
                     0.0, // forgetting_factor
                     0,   // maybe used for TA
                     0);  // path_loss in dB
```
Only the input noise can be changed on command line with the `-s` parameter.

With path loss = 0 set `-s 5` to see a little noise. `-s` is a shortcut to `channelmod.s`. It is expected to enhance the channel modelization flexibility by the addition of more parameters in the channelmod section.

Example to add a very small noise:
```bash
-s 30
```
to add a lot of noise:
```bash
-s 5
```

Example run commands:
```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --parallel-config PARALLEL_SINGLE_THREAD --rfsim --phy-test --rfsimulator.options chanmod --rfsimulator.modelname AWGN

```
# Real time control and monitoring

Add the `--telnetsrv` option to the command line. Then in a new shell, connect to the telnet server, example:
```bash
telnet 127.0.0.1 9090
```
once connected it is possible to monitor the current status:
```bash
channelmod show current
```

see the available channel models:
```bash
channelmod show predef
```

or modify the channel model, for example setting a new model:
```bash
rfsimu setmodel AWGN
```
setting the pathloss, etc...:
```bash
channelmod modify <channelid> <param> <value>
channelmod modify 0 ploss 15
```
where `<param>` can be one of `riceanf`, `aoa`, `randaoa`, `ploss`, `offset`, `forgetf`.

# Caveats

There are issues in power control: txgain/rxgain setting is not supported.
