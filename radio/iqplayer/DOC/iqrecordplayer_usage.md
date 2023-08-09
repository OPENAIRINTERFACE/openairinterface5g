# I/Q record-playback
## using the iq recorder or iq player

This feature provides I/Q record-playback as presented in the 4th OAI workshop. The implementation allows any softmodem executable to use the iq's record/player.
The I/Q record/playback feature is briefly described hereafter, it allows you to:
* record subframes received by the USRP board in a file while the system is operating (for example you can record a full UE connection/traffic/ disconnection sequence). Today the oai USRP device is the only one supporting the recording feature. 
* replay subframes from a file to operate the system (possibly in multiple loops if the recorded sequence is convenient from the logical point of view). 

The record/replay  are activated and configured using oai configuration parameters

### Record mode
options for record mode are:
* `subframes-record` Activate record mode
* `subframes-file` Path of the file used for subframes recording (default is `/tmp/iqfile`)
* `subframes-max` Maximum count of subframes to be recorded in subframe file (default is 120000)

Note that the value of `--subframes-max` parameter needs to be tuned according to your RAM capabilities. The default value is 120000, which allows for 120 seconds of record/replay but will require ~3.6GB of RAM and disk space. 
If RAM does not allow for the parameter value, an error message will be displayed and the run will be aborted. In record mode, to minimize pzerformance impact, 
subframes are saved into memory and written to disk when the process is terminating . When you estimate the recorded sequence is achieved, you should terminate the eNB/RRU by typing `CTRL-C`. At that time, up to the value of `--subframes-max` subframes will be written on disk. The eNB/RRU will indicate the exact count of subframes written to disk, which may be less (but not higher) that the value of `--subframes-max` parameter (120000 by default). The number of subframes written to disk is rounded to an integral number of frames. 
Although it is possible to perform recording with a fronthaul setup, it is suggested to perform recording in full eNB mode and then replay in either monolithic or split eNB.
The option can be used while operating full eNB or RCC/RRU linked by NGFI IF5/IF4p5 fronthaul techniques. One of the advantage of the record/replay technique is that you can replay on a fronthauling setup a file that has been recorded in full eNB mode. This can easily asserts that the fronthauling underlying set up or technique is correct.

>recording session example:
```bash
 ./lte-softmodem -O libconfig:/usr/local/oai/conf/enb.band7.tm1.20PRB.usrpb210.conf:dbgl5    --log_config.global_log_options level,thread   --device.recplay.subframes-record 1
............................................
............................................
[CONFIG] device.recplay.subframes-file set to default value "/tmp/iqfile"
[CONFIG] subframes-record: 0
[CONFIG] device.recplay.subframes-record set to default value
[CONFIG] subframes-replay: 0
[CONFIG] device.recplay.subframes-replay set to default value
[CONFIG] subframes-max: 120000
[CONFIG] device.recplay.subframes-max set to default value
[CONFIG] subframes-loops: 5
[CONFIG] device.recplay.subframes-loops set to default value
[CONFIG] subframes-read-delay: 700
[CONFIG] device.recplay.subframes-read-delay set to default value
[CONFIG] subframes-write-delay: 15
[CONFIG] device.recplay.subframes-write-delay set to default value
[LIBCONFIG] device.recplay: 7/7 parameters successfully set, (7 to default value)
[CONFIG] subframes-record: 1
[CONFIG] device.recplay 1 options set from command line
[CONFIG] loader.oai_device.shlibversion set to default value ""
[LIBCONFIG] loader.oai_device: 2/2 parameters successfully set, (1 to default value)
[CONFIG] loader.oai_device 0 options set from command line
[LOADER] library liboai_device.so successfully loaded
USRP device initialized in subframes record mode
[HW]I lte-softmodem UHD version 3.14.1.1-release (3.14.1)
...................................................
...................................................
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 18, min_I0 12
^C
Run time: 0h 26s
	Time executing user inst.: 1s 865251us
	Time executing system inst.: 2s 805162us
	Max. Phy. memory usage: 4068328kB
	Page fault number (no io): 1008741
	Page fault number (requiring io): 15
	Number of file system read: 97672
	Number of filesystem write: 0
	Number of context switch (process origin, io...): 562871
	Number of context switch (os origin, priority...): 112
Linux signal Interrupt...
/usr/local/oai/develop/openairinterface5g/executables/softmodem-common.c:188 signal_handler() Exiting OAI softmodem: softmodem starting exit procedure
Exiting ru_thread 
U[HW]I ru thread Writing file header to /tmp/iqfile 
[HW]I lte-softmodem Writing file header to /tmp/iqfile 
[HW]I ru thread Writing 19730 subframes to /tmp/iqfile 
[HW]I lte-softmodem Writing 19730 subframes to /tmp/iqfile 
[HW]I lte-softmodem File /tmp/iqfile closed
[HW]I ru thread File /tmp/iqfile closed
[PHY]I ru thread RU 0 rf device stopped
```

### Playback/replay mode
When replaying iq's received by eNB or gNB The option works only for 40MHz bandwidth 3/4 sampling because the information stored in the iq's file header regarding bandwidth is not yet properly processed. In addition the current implementation has only been validated with nr-uesoftmodem in 5G SA configuration.  
 In this version,  mismatch between file content and run time parameters might lead to unpredictable results. In addition a file recorded on a AVX2-capable processor cannot be replayed on a non-AVX2-capable processor (this is to be further investigated).
options for replay mode are:
* `subframes-replay` Activate replay mode
* `subframes-file` Path of the file used for subframes replay (default is `/tmp/iqfile`)
* `subframes-loops` Number of iterations to replay the entire subframes file (default is 5)
* `subframes-read-delay` Delay in microseconds to read a subframe in replay mode (default 200)
* `subframes-write-delay` Delay in milliseconds to write a subframe in replay mode (default 15)
* `use-mmap` Boolean, set to 1 (true) by default, iq file is map to memory if true, otherwise iq's are read from file. 

>iq player session example:
```bash
./lte-softmodem -O libconfig:/usr/local/oai/conf/enb.nbiot.band7.tm1.50PRB.usrpb210.conf:dbgl5  --T_stdout 1   --log_config.global_log_options level,thread --eNBs.[0].component_carriers.[0].N_RB_DL 25   --device.recplay.subframes-replay 1
[CONFIG] get parameters from libconfig /usr/local/oai/conf/enb.nbiot.band7.tm1.50PRB.usrpb210.conf , debug flags: 0x00000005
................................
..................................
[PHY]I lte-softmodem rxdataF[0] 0x55f52df52f40 for RU 0
[CONFIG] device.recplay.subframes-file set to default value "/tmp/iqfile"
[CONFIG] subframes-record: 0
[CONFIG] device.recplay.subframes-record set to default value
[CONFIG] subframes-replay: 0
[CONFIG] device.recplay.subframes-replay set to default value
[CONFIG] subframes-max: 120000
[CONFIG] device.recplay.subframes-max set to default value
[CONFIG] subframes-loops: 5
[CONFIG] device.recplay.subframes-loops set to default value
[CONFIG] subframes-read-delay: 700
[CONFIG] device.recplay.subframes-read-delay set to default value
[CONFIG] subframes-write-delay: 15
[CONFIG] device.recplay.subframes-write-delay set to default value
[LIBCONFIG] device.recplay: 7/7 parameters successfully set, (7 to default value)
[CONFIG] subframes-replay: 1
[CONFIG] device.recplay 1 options set from command line
[CONFIG] loader.oai_iqplayer.shlibversion set to default value ""
[LIBCONFIG] loader.oai_iqplayer: 2/2 parameters successfully set, (1 to default value)
[CONFIG] loader.oai_iqplayer 0 options set from command line
[LOADER] library liboai_iqplayer.so successfully loaded
[HW]I lte-softmodem Loading subframes using mmap() from /tmp/iqfile size=324433632 bytes ...
[HW]I lte-softmodem Replay iqs from USRP B200 device, bandwidth 5.000000e+06
[HW]I lte-softmodem Loaded 10550 subframes.
[HW]I lte-softmodem iqplayer device initialized, replay /tmp/iqfile  for 5 iterations[HW]I lte-softmodem [RAU] has loaded USRP B200 device.
.............................................
.............................................
[PHY]I ru thread max_I0 18, min_I0 12
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 18, min_I0 0
[HW]I ru thread starting subframes file with wrap_count=1 wrap_ts=81024000 ts=107616555
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 18, min_I0 12
[HW]I ru thread starting subframes file with wrap_count=2 wrap_ts=162048000 ts=188640555
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 18, min_I0 0
[HW]I ru thread starting subframes file with wrap_count=3 wrap_ts=243072000 ts=269664555
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 19, min_I0 12
[HW]I ru thread starting subframes file with wrap_count=4 wrap_ts=324096000 ts=350688555
[PHY]I lte-softmodem prach_I0 = 0.0 dB
[PHY]I ru thread max_I0 18, min_I0 12
[HW]W ru thread iqplayer device terminating subframes replay  after 5 iteration
/usr/local/oai/develop/openairinterface5g/radio/iqplayer/iqplayer_lib.c:222 trx_iqplayer_read() Exiting OAI softmodem: replay ended, triggering process termination
```

## iq recorder and iq player implementation overview

### configuration
The iq's record/player is using the [configuration module](../../../../common/config/DOC/config.md). Configuration parameters supported by the record player are defined in the [record_player.h](../../COMMON/record_player.h) include file.
 There are no specific sections for the recorder or the replayer, parameters are defined under the `device.recplay`section and the `read_recplayconfig` function is common to both the recorder and the player. `device.recplay` configuration section is read from oai code common to all devices:
Implemented in [record_player.c](../../COMMON/record_player.c), the `read_recplayconfig` function is called from `load_lib` and reads the configuration parameters, saving them in a `recplay_conf_t` sub-structure of the oai device. It also allocates a `recplay_state_t` structure which will save the recorder or player data. 
3GPP authentication algorithm doesn't allow session establishment via replaying iq's. When using the replayer, a softmodem parameter is set (`SOFTMODEM_RECPLAY_BIT` in the `optmask` bitmask of the `softmodem_params_t` structure. It allows bypassing or activating specific code in the softmodem to allow session establishment replay. This mechanism only works if in parallel specific code is also activated in the EPC, which is supported by the NOKIA LTE box.
An easiest scenario is to use the record player in parallel with the noS1 mode, as described [here](../../../../doc/RUNMODEM.md) 

### iq recorder
most of the implementation is now located in the oai device common code:
* The`load_lib` function in [common-lib.c](../../COMMON/common_lib.c) reads the record player parameters. 
* Recorder specific functions are implemented in  `iqrecorder_end` saves the cached iq's to disk, it has to be called by the oai device in it's end function.
* When recording is enabled, the device must implement iq's caching in it's read function. It also has to call the `iqrecorder_end ` function when  terminating to write iqs to a file on disk. Look at the USRP device ( [../usrp_lib.cpp](../../USRP/USERSPACE/LIB/usrp_lib.cpp) ) for  details.

### iq player
The replay feature is implemented as a oai device. 
* The`load_lib` function in [common-lib.c](../../COMMON/common_lib.c) reads the record player parameters, this code is common with the recorder.
* The player device is implemented in  [iqplayer_lib.c](../iqplayer_lib.c) which at build time creates the `liboai_iqplayer.so` shared library. Thi
* s device is automatically, dynamicaly loaded when the player feature is activated at runtime when the boolean  `device.recplay.subframes-replay` option is set.

### iq's file format
The recorder adds a header containing the device type, the bandwith and a format identifier. Using these information when replaying a file has not yet been implemented, so the player only supports USRP B2xx device and 40MHz bandwith 3/4 sampling. 

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
