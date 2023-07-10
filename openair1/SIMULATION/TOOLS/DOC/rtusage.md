## configuring the channel simulation
Oai channel simulation is using the [config module](../../../config/config.md) to get its parameters at init time. The [telnet server](../../telnetsrv/DOC/telnetsrv.md) includes a set of commands which can be used to dynamically modify some channel model parameters

All channel simulation parameters are defined in the `channelmod` section. Most parameters are specific to a channel model and  are only used by the rfsimulator.

### global parameters

| name | type | default | description |
|:---:|:---:|:---:|:----|
| `max_chan` | integer | `25` | Maximum number of channel model that can be defined in the system. Must be greater than the number of model definitions  in the model list loaded at init time. |
| `modellist` | character string | `DefaultChannelList` | Name of the channel models list to load at init time. |

### Model lists

Several model lists can be defined in the oai configuration file. One, defined by the `modellist` parameter is loaded at init time. In the configuration file each model list item describes a channel model using a group of parameters:

| parameter name | type | default | description |
|:---:|:---:|:---:|:----|
| `model name` | character string | mandatory |name of the model, as used in the code to retrieve a model definition|
| `type` |                  | `AWGN` | name of the channel modelization algorithm applied on rf signal. The list of available model types can be listed via the [telnet server](../../telnetsrv/DOC/telnetsrv.md) or by entering an invalid type name |
| `ploss_dB` |   real (float)   |  | path loss of the channel, in dB |
| `noise_power_dB` |  real (double)   |  | noise of the channel in dB |
| `forgetfact` |  real (double)   |  |  |
| `offset` |     integer      |  |  |
| `ds_tdl` |   real double    |  |  |

Channel simulation parameters can also be specified on the command line:

```bash
./lte-softmodem -O ../../../ci-scripts/conf_files/enb.band7.tm1.50prb.usrpb210.conf --noS1 --rfsim --rfsimulator.options chanmod --rfsimulator.serveraddr enb --telnetsrv --channelmod.modellist modellist_rfsimu_2 --channelmod.modellist_rfsimu_2.[1].offset 120
```
### Using the telnet server to modify channel simulator parameters
The telnet server includes a `channelmod` command which can be used to dynamically modify some channel model parameters. This command is only available when channel simulation is enabled (via `rfsimulator.options chanmod` option when running the rfsimulator.  `channelmod` command has its own help:

```
$ telnet 127.0.0.1 9090
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
 
softmodem_enb> help
.....................................
   module 6 = channelmod:
      channelmod help 
      channelmod show <predef,current>
      channelmod modify <channelid> <param> <value>
   module 7 = rfsimu:
      rfsimu setmodel <model name> <model type>
softmodem_enb> channelmod help
channelmod commands can be used to display or modify channel models parameters
channelmod show predef: display predefined model algorithms available in oai
channelmod show current: display the currently used models in the running executable
channelmod modify <model index> <param name> <param value>: set the specified parameters in a current model to the given value
                  <model index> specifies the model, the show current model command can be used to list the current models indexes
                  <param name> can be one of "riceanf", "aoa", "randaoa", "ploss", "noise_power_dB", "offset", "forgetf"
softmodem_enb> 

```







The [rfsimulator documentation](../../../../radio/rfsimulator/README.md ) has also some specific information when using the channel simulation via this tool. 

[channel simulation main page](channel_simulation.md)
[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
