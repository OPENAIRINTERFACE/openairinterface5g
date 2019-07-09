#General
This is a RF simulator that allows to test OAI without a RF board.
It replaces a actual RF board driver.

As much as possible, it works like a RF board, but not in realtime: it can run faster than realtime if there is enough CPU or slower (it is CPU bound instead of real time RF sampling bound)

#build

## From build_oai
You can build it the same way, and together with actual RF driver

Example:
```bash
./build_oai --ue-nas-use-tun --UE --eNB -w SIMU
```
It is also possible to build actual RF and use choose on each run:
```bash
./build_oai --ue-nas-use-tun --UE --eNB -w USRP --rfsimulator
```
Will build both the eNB (lte-softmodem) and the UE (lte-uesoftmodem)
We recommend to use the option --ue-nas-use-tun that is much simpler to use than the OAI kernel driver.

## Add the rfsimulator after initial build
After any regular build, you can compile the driver
```bash
cd <the_compilation_dir_from_bouild_oai_script>/build
make rfsimulator
```
Then, you can use it freely

# Usage
Setting the env variable RFSIMULATOR enables the RF board simulator
It should the set to "server" in the eNB or gNB

## 4G case
For the UE, it should be set to the IP address of the eNB
example: 
```bash
sudo RFSIMULATOR=192.168.2.200 ./lte-uesoftmodem -C 2685000000 -r 50 
```
Except this, the UE and the eNB can be used as it the RF is real

If you reach 'RA not active' on UE, be careful to generate a valid SIM
```bash
$OPENAIR_DIR/targets/bin/conf2uedata -c $OPENAIR_DIR/openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o .
```
## 5G case
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

If you set the environment variable: saveIQfile to a file name
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
The RF channel simulator is called.
In current version all channel paramters are hard coded in the call to:
```
new_channel_desc_scm(bridge->tx_num_channels,bridge->rx_num_channels,
                                          AWGN,
                                          bridge->sample_rate,
                                          bridge->tx_bw,
                                          0.0, // forgetting_factor
                                          0, // maybe used for TA
                                          0); // path_loss in dB
```
Only the input noise can be changed on command line with -s parameter.
With path loss = 0 set "-s 5" to see a little noise

#Caveacts
Still issues in power control: txgain, rxgain are not used
