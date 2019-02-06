#General
This is a RF simulator that allows to test OAI without a RF board.
It replaces a actual RF board driver.

As much as possible, it works like a RF board, but not in realtime: it can run faster than realtime if there is enough CPU or slower (it is CPU bound instead of real time RF sampling bound)

#build 
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

#usage
Setting the env variable RFSIMULATOR enables the RF board simulator
It should the set to "enb" in the eNB

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
#Caveacts
Still issues in power control: txgain, rxgain are not used

no S1 mode is currently broken, so we were not able to test the simulator in noS1 mode
