[[_TOC_]]

# General usage

`-O` is the only mandatory command line option to start the softmodems (4G/5G xNB, UEs). It is used to specify the configuration source with the associated parameters:
```bash
$ ./lte-softmodem -O <configsource>:<parameter1>:<parameter2>:...
```
The same applies for `nr-softmodem`, `lte-uesoftmodem`, `nr-uesoftmodem`.

The configuration module can also be used without a configuration source, ie to only parse the command line. In this case the -O switch is optional. This mode is typically used in the `lte-uesoftmodem` executable and by the physical simulator executables (`ulsim`, `dlsim`).

# Configuration sources

Currently the available config sources are:

- **libconfig**: libconfig file. [libconfig file format](http://www.hyperrealm.com/libconfig/libconfig_manual.html#Configuration-Files) Parameter1 is the file path and parameter 2 can be used to specify the level of console messages printed by the configuration module.
```bash
$ ./lte-softmodem -O libconfig:<config>:dbgl<debuglevel>
```
- **cmdlineonly**: command line only, the default mode for lte-uesoftmodem and the phy simiulators. In this case -O may be used to specify the config module debug level.

# Detailed configuration information and debugging

The debug level is a mask:
*  bit 1: print parameters values
*  bit 2: print memory allocation/free performed by the config module
*  bit 3: print command line processing messages
*  bit 4: disable execution abort when parameters checking fails
*  bit 5: write a config file  reflecting the parameters after reading the configuration file and processing the command line

As a oai user, you may have to use bit 1 (dbgl1) , to check your configuration and get the full name of a parameter you would like to modify on the command line. Other bits are for developers usage, (dbgl7 will print all debug messages).

```bash
$ ./lte-softmodem -O libconfig:<config>:dbgl1
```
```bash
$ ./lte-uesoftmodem -O cmdlineonly:dbgl1
```
bit 5 (dbgl32) can be useful to detect unused parameters from a config file or to detect parameters set to their default values, and not present in the config file

To get help on supported parameters you can use specific options:

*  `--help`: print help for command line only parameters and for parameters not defined in a specific section
	*  `--help_<prefix>` : print help for parameters defined under the section `<prefix>`

```
./lte-softmodem -O libconfig:/usr/local/oai/conf/enb.nbiot.band7.tm1.50PRB.usrpb210.conf   --help
[CONFIG] get parameters from libconfig /usr/local/oai/conf/enb.nbiot.band7.tm1.50PRB.usrpb210.conf , debug flags: 0x00000000
.............................................
[LIBCONFIG] (root): 19/19 parameters successfully set, (16 to default value)

-----Help for section (root section)            : 019 entries------
    --rf-config-file: Configuration file for front-end (e.g. LMS7002M)
    --ulsch-max-errors: set the eNodeB max ULSCH erros
    --phy-test: test UE phy layer, mac disabled
    --usim-test: use XOR autentication algo in case of test usim mode
    --emulate-rf: Emulated RF enabled(disable by defult)
    --clock: tells hardware to use a clock reference (0:internal, 1:external, 2:gpsdo)
    --wait-for-sync: Help string not specified
    --single-thread-enable: Disables single-thread mode in lte-softmodem
    -C: Set the downlink frequency for all component carriers
    -a: Channel id offset
    -d: Enable soft scope and L1 and L2 stats (Xforms)
    -q: Enable processing timing measurement of lte softmodem on per subframe basis
    -S: Skip the missed slots/subframes
    --numerology: adding numerology for 5G
    --parallel-config: three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'
    --worker-config: two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'
    --nbiot-disable: disable nb-iot, even if defined in config
    --noS1: Disable s1 interface
    --nokrnmod: (noS1 only): Use tun instead of namesh module
--------------------------------------------------------------------

[LIBCONFIG] (root): 4/4 parameters successfully set, (4 to default value)

-----Help for section (root section)            : 004 entries------
    -R: Enable online log
    -g: Set the global log level, valide options: (4:trace, 3:debug, 2:info, 1:warn, (0:error))
    --telnetsrv: Start embedded telnet server
--------------------------------------------------------------------

[LIBCONFIG] loader: 2/2 parameters successfully set, (2 to default value)
[LIBCONFIG] loader.telnetsrv: 2/2 parameters successfully set, (1 to default value)
[LOADER] library libtelnetsrv.so is not loaded: libtelnetsrv.so: cannot open shared object file: No such file or directory
Getting ENBSParams
[LIBCONFIG] (root): 3/3 parameters successfully set, (1 to default value)

-----Help for section (root section)            : 003 entries------
    --Asn1_verbosity: Help string not specified
    --Active_eNBs: Help string not specified
    --noS1: Help string not specified
--------------------------------------------------------------------

/usr/local/oai/issue390_configmodule_cmdlinebug/openairinterface5g/common/config/config_cmdline.c:224 config_process_cmdline() Exiting OAI softmodem: [CONFIG] Exiting after displaying help
```

For the lte-softmodem (the eNodeB), the config source parameter defaults to libconfig, preserving the initial -O option format. In this case you cannot specify the debug level.

```bash
$ ./lte-softmodem -O <config>
```

# Configuration of the configuration module

Configuration file parameters, except for the configuration file path,  can be specified in a **config** section in the configuration file:

```
config:
{
    debugflags = 1;
}
```

Configuration files examples can be found in the `targets/PROJECTS/GENERIC-LTE-EPC/CONF` sub-directory of the oai source tree. 

# Overwriting config options from the command line

To minimize the number of configuration file to maintain, any parameter can also be specified on the command line. For example to modify the lte bandwidth to 20 MHz where the configuration file specifies 10MHz you can enter:

```bash
$ ./lte-softmodem -O <config> --eNBs.[0].component_carriers.[0].N_RB_DL 100
```

As specified earlier, use the dbgl1 debug level to get the full name of a parameter you would like to modify on the command line.

[Configuration module home](../config.md)
