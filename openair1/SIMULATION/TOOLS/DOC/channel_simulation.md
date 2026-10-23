<!-- SPDX-License-Identifier: CC-BY-4.0 -->

[[_TOC_]]

This document is complementary to the [RFSIMULATOR Tutorial](../../../../radio/rfsimulator/README.md).

# Channel Modeling

Channel models in the context of wireless communication refer to mathematical models that simulate the effects of transmission mediums on signal propagation. These models account for factors such as attenuation, interference, and fading, which can affect the quality of communication between transmitter and receiver.

Different channel models represent different real-world scenarios, such as urban environments, indoor spaces, or rural areas. By using these models, researchers and engineers can predict and evaluate the performance of wireless systems under various conditions.

# OAI channel simulation feature

OpenAirInterface RFSimulator incorporates a channel simulation feature. This feature allows any component to modify the time domain samples of a RF channel. It achieves this by applying predefined models, such as those defined in 3GPP TR 36.873 or TR 38.901.

The definition, configuration, and real-time modification of a channel model are implemented in a common code. This code is included in UE, gNB and eNB. It is utilized when operating with the RFSimulator or the L1 simulator. PHY simulators also employ channel simulation, but their configuration is accomplished via dedicated command-line options.

The RFSimulator is the exclusive option that provides access to all the configuration and real-time modification features of OAI's channel simulation. This makes it a comprehensive tool for managing and manipulating channel simulations in OAI.

# Implementation
OAI channel simulation is using the [config module](../../../../common/config/DOC/config.md) to get its parameters at init time. The [telnet server](../../../../common/utils/telnetsrv/DOC/telnetsrv.md) includes a set of commands which can be used to dynamically modify some channel model parameters.

The relevant source files are:

1. [`radio/rfsimulator/simulator.cpp`](../../../../radio/rfsimulator/simulator.cpp)
2. [`radio/rfsimulator/apply_channelmod.c`](../../../../radio/rfsimulator/apply_channelmod.c)
3. [`random_channel.c`](../random_channel.c)
4. [`sim.h`](../sim.h)

All channel parameters are set depending on the model name via a call to:

```bash
channel_desc_t *new_channel_desc_scm(uint8_t nb_tx,
                                     uint8_t nb_rx,
                                     SCM_t channel_model,
                                     double sampling_rate,
                                     uint64_t center_freq,
                                     double channel_bandwidth,
                                     double DS_TDL,
                                     double maxDoppler,
                                     const corr_level_t corr_level,
                                     double forgetting_factor,
                                     int32_t channel_offset,
                                     double path_loss_dB,
                                     float noise_power_dB);
```

# Channel Model configuration file

To define and use a channel model for uplink, the gNB configuration file needs to include a channel configuration file. To do this, add:

```bash
@include "channelmod_rfsimu.conf"
```

at the end of the gNB configuration file, and place the channel configuration file in the same directory. The same shall be done for downlink by including the channel model configuration file at the end of UE configuration file. E.g.

* nrUE [`ci-scripts/conf_files/nrue.uicc.conf`](../../../../ci-scripts/conf_files/nrue.uicc.conf)
* gNB [gnb.sa.band78.106prb.rfsim.conf](../../../../ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.conf)

All channel simulation parameters are defined in the `channelmod` section. Most parameters are specific to a channel model and  are only used by the rfsimulator. An example of the configuration file can be found here:

* [`ci-scripts/conf_files/channelmod_rfsimu.conf`](../../../../ci-scripts/conf_files/channelmod_rfsimu.conf)

e.g. a simple scenario to with an AWGN channel:

```bash
channelmod = {
  max_chan = 10;
  modellist = "modellist_rfsimu_1";
  modellist_rfsimu_1 = (
    {
      model_name     = "rfsimu_channel_enB0"
      type           = "AWGN";
      ploss_dB       = 0;
      noise_power_dB = -14;
      forgetfact     = 0;
      offset         = 0;
      ds_tdl         = 0;
    },
    {
      model_name     = "rfsimu_channel_ue0"
      type           = "AWGN";
      ploss_dB       = 0;
      noise_power_dB = -12;
      forgetfact     = 0;
      offset         = 0;
      ds_tdl         = 0;
    }
  );
};
```

where `rfsimu_channel_ue0` will be activated on server side for uplink and `rfsimu_channel_enB0` will be activated on client side for downlink.

Use `rfsimu_channel_ue1`, `rfsimu_channel_ue2`, etc. if you want to use different channel models for each client. The client connection order determines its channel model.

The server could be either the UE or the gNB, the channel name suffix does not depend on the application but on the rfsimulators role (server/client).

## Edit the configuration file

How simulate noise of a AWGN channel? Starting from the configuration file in the previous section, in the `rfsimu_channel_enB0` model part, set the noise power to:

```bash
  noise_power_dB  = -20
```

In the `rfsimu_channel_ue0` model part, set the noise power to:
```bash
  noise_power_dB  = -30;
```

and the rest of the `channelmod_rfsimu.conf` remains unchanged.

## Monitoring by nr-scope

In order to verify the effects of the changes introduced by the channel simulation, the `nr-scope` constellation tool can be used to track and analyze the modulation constellation points. This tool allows users to visualize the modulation scheme being used and assess the quality of the received signals. By observing the constellation points, users can verify whether the changes made to the system configuration have resulted in.

# Build the telnet server library

Please refer to this documentation [telnetusage.md](../../../../common/utils/telnetsrv/DOC/telnetusage.md).

# Run OAI with a channel model

When the `chanmod` option is enabled, the RF channel simulator with a channel modeling function is called. Add the following options to the command line to enable the channel model:
```bash
--rfsimulator.[0].options chanmod
```

Example run of OAI RFSIM on the same machine with activation of the channel model via command line:
gNB:
```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --gNBs.[0].min_rxtxtime 6 --rfsim --rfsimulator.[0].options chanmod --telnetsrv
```
UE:
```bash
sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --rfsimulator.[0].serveraddr 127.0.0.1 --uicc0.imsi 001010000000001 -O ../../../ci-scripts/conf_files/nrue.uicc.conf --rfsimulator.[0].options chanmod
```
where `@include "channelmod_rfsimu.conf"` has been added at the end of `ci-scripts/conf_files/channelmod_rfsimu.conf` which has been copied to `targets/PROJECTS/GENERIC-LTE-EPC/CONF/`.

## Set channel simulation parameters via CL:

Channel simulation parameters can also be specified on the command line by using the `--channelmod` option:

### Global parameters
| CL option           |type        |default             | description |
|:---                 |:----       |:----               |:----|
|`modellist`          |char string |`DefaultChannelList`|select and load the `modellist` from the config file.|
|`max_chan`           |integer     |10                  |set the maximum number of channel models that can be defined in the system. Must be greater than the number of model definitions in the model list loaded at init time.|
|`noise_power_dBFS`   |integer     |0                   |Noise power in dBFS. If set, noise per channel is not applied. To achieve positive SNR use values below the default gNB/nrUE amp backoff value (-36dBFS)|

Example usage:
```bash
--channelmod.modellist modellist_rfsimu_2
```

### Model lists
Several model lists can be defined in the OAI channel simulation configuration file. One, defined by the `modellist` parameter is loaded at init time. In the configuration file each model list item describes a channel model using a group of parameters:

|Parameter name   |Type         |Default    |Description |
|:---             |:---:        |---:       |:----       |
| `model_name`    |char string  | mandatory |name of the model, as used in the code to retrieve a model definition.|
| `type`          |char string  | `AWGN`    |name of the channel modelization algorithm applied on RF signal. The list of available models is defined in [`sim.h`](../sim.h]|
| `ploss_dB`      |real (float) |          0|net coupling loss of the link in dB: propagation losses (including shadow fading) minus TX/RX antenna gains. Positive values attenuate the signal; a negative value is a net gain |
| `noise_power_dB`|real (double)|        -50|Noise power in dB, used to compute the SNR. Its value is proportional to the noise added.|
| `forgetfact`    |real (double)|          0|Forgetting factor, allows for simple 1st order temporal variation. 0 means a new channel every call, 1 means keep channel constant all the time|
| `offset`        |integer      |          0|channel offset for accessing the input signal, in samples|
| `ds_tdl`        |real double  |          0|delay spread for TDL models|

`ploss_dB` and `noise_power_dB` are applied to each IQ sample in order to scale the received signal: the signal amplitude is multiplied by `10^(-ploss_dB/20)` and white Gaussian noise of standard deviation `256 * 10^(noise_power_dB/10)` is added. Increasing `ploss_dB` by 1 dB lowers the SNR by 1 dB; increasing `noise_power_dB` by 1 dB lowers it by 2 dB.

Example usage, set the offset for the selected model of the selected `modellist`:
```bash
--channelmod.<modellist>.[<model ID>].offset
```

e.g. with the softmodem:
```bash
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --gNBs.[0].min_rxtxtime 6 --rfsim --rfsimulator.[0].options chanmod --telnetsrv --channelmod.modellist modellist_rfsimu_2 --channelmod.modellist_rfsimu_2.[1].offset 120
```

## Real time control and monitoring with telnet server
Add the `--telnetsrv` option to the command line. Then in a new shell, connect to the telnet server, e.g.:

```bash
$ telnet 127.0.0.1 9090
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'
```
to enter the command and control of the simulator press any key: `softmodem_enb` will be displayed for the eNB/gNB and `softmodem_5Gue` will be displayed for the UE. Select a different port in order to use both servers on the same machine.

The telnet server includes two modules to change the RF channel simulation in real-time:

1. `channelmod` to dynamically modify the channel model configuration parameters
2. `rfsimu` to set a different channel model

They both have their own help:
```bash
softmodem_enb> help
.....................................
   module 4 = channelmod:
      channelmod help
      channelmod show <predef,current>
      channelmod modify <channelid> <param> <value>
      channelmod show params <channelid> <param> <value>
   module 5 = rfsimu:
      rfsimu setmodel <model name> <model type>
      rfsimu setdistance <model name> <distance>
      rfsimu getdistance <model name>
      rfsimu vtime
softmodem_enb> channelmod help
channelmod commands can be used to display or modify channel models parameters
channelmod show predef: display predefined model algorithms available in oai
channelmod show current: display the currently used models in the running executable
channelmod modify <model index> <param name> <param value>: set the specified parameters in a current model to the given value
                  <model index> specifies the model, the show current model command can be used to list the current models indexes
                  <param name> can be one of "riceanf", "aoa", "randaoa", "ploss", "noise_power_dB", "offset", "forgetf"
softmodem_enb>
```

### channelmod module
Example usage:

| Command                                       |Description                      |
|:---                                           |:----                            |
|`channelmod show current`                      |Monitor the current status       |
|`channelmod show predef`                       |See the available channel modelss|
|`channelmod modify <channelid> <param> <value>`|Set parameters specific to the channel model, e.g. pathloss, Ricean factor|

Where `<param>` in the `channelmod modify` command can be:

|Parameter       |Type  |Range    |Description |
|:---            |:---: |:---:    |:----       |
|`riceanf`       |double|0 to 1   |Ricean factor, of first tap w.r.t. other taps (where 0 means AWGN and 1 means Rayleigh channel)|
|`aoa`           |double|0 to 2*Pi|Angle of arrival of wavefront (in radians). For Ricean channel only. This assumes that both RX and TX have linear antenna arrays with lambda/2 antenna spacing. Also it is assumed that the arrays are parallel to each other and that they are far enough apart so that we can safely assume plane wave propagation.      |
|`randaoa`       |bool  |0 or 1   |randomized angle of arrival according to a uniform random distribution|
|`ploss`         |double|-        |same as ploss_dB in [Model lists](#model-lists) (coupling loss in dB, positive = attenuation)|
|`noise_power_dB`|int   |-        |same as noise_power_dB in [Model lists](#model-lists)|
|`forgetf`       |double|0 to 1   |same as forgetfact in [Model lists](#model-lists)|

Example usage:
```bash
channelmod modify 0 ploss 12
channelmod modify 0 noise_power_dB 3
```

### rfsimu module
This module can be used to set a different channel model, e.g. `rfsimu setmodel AWGN`.
