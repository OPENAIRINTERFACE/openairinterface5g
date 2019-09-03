# General
This is a RF simulator that allows to test OAI without a RF board.
It replaces a actual RF board driver.

As much as possible, it works like a RF board, but not in realtime: it can run faster than realtime if there is enough CPU or slower (it is CPU bound instead of real time RF sampling bound)

# build

## From [build_oai](../../../doc/BUILD.md) script
The RF simulator is implemented as an oai device and is always build when you build the oai eNB or the oai UE.
Using the `-w SIMU` option it is possible to just re-build the RF simulator device 

Example:
```bash
./build_oai --UE --eNB
Will compile UE
Will compile eNB
CMAKE_CMD=cmake ..
No local radio head and no transport protocol selected
No radio head has been selected (HW set to None)
No transport protocol has been selected (TP set to None)
RF HW set to None
Flags for Deadline scheduler: False
..................
.................
Compiling rfsimulator
Log file for compilation has been written to: /usr/local/oai/rfsimu_config/openairinterface5g/cmake_targets/log/rfsimulator.Rel14.txt
rfsimulator compiled
......................
......................
```

## Add the rfsimulator after initial build
After any regular build, you can compile the device, from the build directory
```bash
cd <path to oai sources>/openairinterface5g/cmake_targets/lte_build_oai/build
make rfsimulator
```
this is equivalent to using `-w SIMU` when running the `build_oai` script.
e 

# Usage
To use the RF simulator you add the  `--rfsim` option to the command line. By default the RF simulator device will try to connect to host 127.0.0.1, port 4043, which is usually the behavior for the UE.
The RF simulator is using the configuration module, its parameters are defined in a specific section called "rfsimulator"

| parameter            | usage                                                                                                             | default |
|:---------------------|:------------------------------------------------------------------------------------------------------------------|----:|
| serveraddr           | ip address to connect to, or "enb" to behave as a tcp server                                                      | 127.0.0.1 |
| serverport           | port number to connect to or to listen on (eNB, which behaved as a tcp server)                                    | 4043 |
| options              | list of comma separated run-time options, two are supported: `chanmod` to enable channel modeling and `saviq` to write transmitted iqs to a file | all options disabled  |
| modelname            | Name of the channel model to apply on received iqs when the `chanmod` option is enabled                           | AWGN |
| IQfile               | Path to the file to be used to store iqs, when the `saviq`option is enabled                                       | /tmp/rfsimulator.iqs |
        
Setting the env variable RFSIMULATOR can be used instead of using the serveraddr parameter, it is to preserve compatibility with previous version.

## 4G case
For the UE, it should be set to the IP address of the eNB
example: 
```bash
sudo RFSIMULATOR=192.168.2.200 ./lte-uesoftmodem -C 2685000000 -r 50 
```
Except this, the UE and the eNB can be used as it the RF is real. noS1 mode can also be used with the RF simulator.

If you reach 'RA not active' on UE, be careful to generate a valid SIM. 
```bash
$OPENAIR_DIR/targets/bin/conf2uedata -c $OPENAIR_DIR/openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o .
```
## 5G case
5G RF simulator will be aligned with 4G as the effort to merge 5G specifuc branches into develop is making progress.
After regular build, add the simulation driver
(don't use ./build_oai -w SIMU until we merge 4G and 5G branches)
```bash
cd ran_build/build
make rfsimulator
```
### Launch gNB in one window
```bash
sudo RFSIMULATOR=server ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --parallel-config PARALLEL_SINGLE_THREAD
```
### Launch UE in another window
```bash
sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --numerology 1 -r 106 -C 3510000000 
```
Of course, set the gNB machine IP address if the UE and the gNB are not on the same machine
In UE, you can add "-d" to get the softscope

### store and replay

You can store emitted I/Q samples:

If you set the option `saviq`
The simulator will write all IQ samples into this file

Then, you can replay with the executable "replay_node"

First compile it, as the other binaries
```
make replay_node
```
You can use this binary as I/Q data source to feed whatever UE or NB with recorded I/Q samples.

The file format is successive blocks of a header followed by the I/Q array.
If you have existing stored I/Q, you can adpat the tool "replay_node" to convert your format to the rfsimulator format.

The format intend to be compatible with the OAI store/replay feature on USRP

### Channel simulation
When the `chanmod` option is enabled, The RF channel simulator is called.
In current version all channel paramters are set depending on the model name via a call to:
```
new_channel_desc_scm(bridge->tx_num_channels,bridge->rx_num_channels,
                                          <model name>,
                                          bridge->sample_rate,
                                          bridge->tx_bw,
                                          0.0, // forgetting_factor
                                          0, // maybe used for TA
                                          0); // path_loss in dB
```
Only the input noise can be changed on command line with -s parameter.
With path loss = 0 set "-s 5" to see a little noise. -s is a shortcut to `channelmod.s`. It is expected to enhance the channel modedelization flexibility via the addition of more parameters in the channelmod section.

# Caveacts
Still issues in power control: txgain, rxgain are not used
