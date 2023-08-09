# I/Q record-replay
## using the iq record-replay

This feature provides I/Q record-replay as initially presented in the 4th OAI workshop for LTE
(https://www.openairinterface.org/docs/workshop/4_OAI_Workshop_20171107/Talks/MONGAZON_Nokia-Bell-Labs-OAI-IQ.pdf)
The current implementation has been temporarily disrupted for LTE and only works for 5G SA
nr-uesoftmodem executable.

The I/Q record-replay feature is briefly described hereafter, it allows you to:
* record slots received by the USRP board in a file while the system is operating.
  For example you can record a full nrUE connection/traffic/disconnection sequence.
  Today the OAI USRP device is the only one supporting the recording feature. 
* replay slots from a previously recorded file to operate the system,
  possibly in multiple loops if the recorded sequence is convenient from the logical point of view. 

The record-replay features are activated and configured using OAI configuration parameters

### Record mode
options for record mode are:
* `subframes-record` Activate record mode
* `subframes-file`   Path of the file used for slots recording (default is `/tmp/iqfile`)
* `subframes-max`    Maximum count of slots to be recorded in a file (default is 120000)

Note that the value of `--subframes-max` parameter needs to be tuned according to your RAM capabilities.
The default value is 120000, which allows for 120 seconds of record-replay but will require ~20GB of RAM and disk space. 
If RAM does not allow for the parameter value, an error message will be displayed and the run will be aborted.
In record mode, to minimize performance impacts of operating software, slots are always saved into memory and
written to disk when the process terminates.
When you estimate the recorded sequence is achieved, you can terminate the nr-uesoftmodem by typing `CTRL-C`.
At that time, up to the value of `--subframes-max` slots will be written on disk.
The nr-uesoftmodem will indicate the exact count of slots written to disk, which may be less (but not higher)
than the value of `--subframes-max` parameter.

>Recording session example:
```bash
./nr-uesoftmodem -O /home/oaitests/mediatek_sim.conf --sa --nokrnmod 1 --numerology 1 -r 106 -C 3649440000 --band 78 -E --ue-fo-compensation --device.recplay.subframes-record 1 --device.recplay.subframes-file /home/iqs/oai-nrUE-17042023.dat --device.recplay.use-mmap 1 --device.recplay.subframes-max 30000
............................................
............................................
............................................
^C
[HW]   Writing file header to /home/iqs/oai-nrUE-17042023.dat 
[HW]   Writing 4565 subframes to /home/iqs/oai-nrUE-17042023.dat 
[HW]   File /home/iqs/oai-nrUE-17042023.dat closed
[HW]   releasing USRP

### Replay mode
Replaying I/Q works only for nr-uesoftmodem at 40MHz bandwidth 3/4 sampling.
Mismatch between file content and run time parameters might lead to unpredictable results.
options for replay mode are:
* `subframes-replay` Activate replay mode
* `subframes-file`   Path of the file used for slots replay (default is `/tmp/iqfile`)
* `subframes-loops`  Number of iterations to replay the entire slots file (default is 5)
* `use-mmap`         Boolean, set to 1 (true) by default, iq file is map to memory if true, otherwise iq's are read from file. 

>Replay mode session example:
```bash
./nr-uesoftmodem -O /home/oaitests/mediatek_sim.conf --sa --nokrnmod 1 --numerology 1 -r 106 -C 3649440000 --band 78 -E --ue-fo-compensation --device.recplay.subframes-replay 1 --device.recplay.subframes-file /home/iqs/oai-nrUE-17042023.dat --device.recplay.use-mmap 1 --device.recplay.subframes-loops 1
..................................
..................................
[HW]   Replay iqs from USRP B200 device, bandwidth 4.000000e+07
[HW]   Loaded 4565 subframes.
[HW]   iqplayer device initialized, replay /home/iqs/oai-nrUE-17042023.dat for 1 iteration(s)
..................................
..................................

[OAI Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
