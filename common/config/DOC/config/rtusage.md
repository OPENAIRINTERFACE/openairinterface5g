 
   -O  is the only mandatory command line option to start the eNodeb softmodem (lte-softmodem executable), it is used to specify the configuration source with the associated parameters:  
```bash
$ ./lte-softmodem -O <configsource>:<parameter1>:<parameter2>:...
```
  The configuration module can also be used without a configuration source, ie to only parse the command line. In this case the -O switch is optional. This mode is used in the ue-softmodem executable and by the phy_simulators executables (ulsim, dlsim)  

Currently the available config sources are:

- **libconfig**: libconfig file. [libconfig file format](http://www.hyperrealm.com/libconfig/libconfig_manual.html#Configuration-Files) Parameter1 is the file path and parameter 2 can be used to specify the level of console messages printed by the configuration module.  
```bash
$ ./lte-softmodem -O libconfig:<config>:dbgl<debuglevel>
```
- **cmdlineonly**: command line only, the default mode for lte-uesoftmodem and the phy simiulators. In this case -O may be used to specify the config module debug level.

The debug level is a mask:  
*  bit 1: print parameters values 
*  bit 2: print memory allocation/free performed by the config module
*  bit 3: print command line processing messages
*  bit 4: disable execution abort when parameters checking fails

As a oai user, you may have to use bit 1 (dbgl1) , to check your configuration and get the full name of a parameter you would like to modify on the command line. Other bits are for developers usage, (dbgl7 will print all debug messages).  

```bash
$ ./lte-softmodem -O libconfig:<config>:dbgl1  
```
```bash
$ ./lte-uesoftmodem -O cmdlineonly:dbgl1
```
For the lte-softmodem (the eNodeB) The config source parameter defaults to libconfig, preserving the initial -O option format. In this case you cannot specify the debug level.  

```bash
$ ./lte-softmodem -O <config>
```

Configuration file parameters, except for the configuration file path,  can be specified in a **config** section in the configuration file:  

```
config:
{
    debugflags = 1;
}
```
Configuration files examples can be found in the targets/PROJECTS/GENERIC-LTE-EPC/CONF sub-directory of the oai source tree. To minimize the number of configuration file to maintain, any parameter can also be specified on the command line. For example to modify the lte bandwidth to 20 MHz where the configuration file specifies 10MHz you can enter:

```bash
$ ./lte-softmodem -O <config> --eNBs.[0].component_carriers.[0].N_RB_DL 100
```  

As specified earlier, use the dbgl1 debug level to get the full name of a parameter you would like to modify on the command line.

[Configuration module home](../config.md)