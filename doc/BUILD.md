<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Build Procedures</font></b>
    </td>
  </tr>
</table>

This page is valid on tags starting from **`2019.w09`**.

# Soft Modem Build Script

oai EPC is developed in a distinct project with it's own [documentation](https://github.com/OPENAIRINTERFACE/openair-cn/wiki) , it is not described here.

OAI UE and eNodeB sources can be downloaded from the Eurecom [gitlab repository](./GET_SOURCES.md).

Sources come with a build script [build_oai](../cmake_targets/build_oai) located at the root of the `openairinterface5g/cmake_targets` directory. This script is developed to build the oai binaries (executables,shared libraries) for different hardware platforms, and use cases. 

The main oai binaries, which are tested by the Continuous Integration process are:

-  The LTE UE: `lte-uesoftmodem`
-  The LTE eNodeB: `lte-softmodem`
-  The PHY simulators: `dlsim` and `ulsim`

The build system for OAI uses [cmake](https://cmake.org/) which is a  tool to generate makefiles. The `build_oai` script is a wrapper using cmake, make and standard linux shell commands to ease the oai build and use . The file describing how to build the executables from source files is the [CMakeLists.txt](../cmake_targets/CMakeLists.txt), it is used as input by cmake to generate the makefiles.

The oai softmodem supports many use cases, and new ones are regularly added. Most of them are accessible using the configuration file or the command line options and continuous effort is done to avoid introducing build options as it makes tests and usage more complicated than run-time options. The following functionalities, originally requiring a specific build are now accessible by configuration or command line options:

- s1, noS1
- all simulators, with exception of PHY simulators, which are distinct executables.


Calling the `build_oai` script with the -h option gives the list of all available options, but a process to simplify and check the requirements of all these options is on-going. Check the 

[table]: BUILD.md	"`build_oai` options"

At the end of this page to know the status of `buid_oai` options which are not described hereafter.

# Building PHY Simulators

Detailed information about these simulators can be found [in this dedicated page](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/OpenAirLTEPhySimul)

# Building UE and eNodeB Executables

After downloading the source files, a single build command can be used to get the binaries supporting all the oai softmodem use cases (UE and eNodeB):

```
cd <your oai installation directory>/openairinterface5g/
source oaienv
cd ../cmake_targets/
./build_oai -I -w USRP --eNB --UE
```

- The `-I` option is to install pre-requisites, you only need it the first time you build the softmodem or when some oai dependencies have changed. 
- The `-w` option is to select the radio head support you want to include in your build. Radio head support is provided via a shared library, which is called the "oai device" The build script creates a soft link from `liboai_device.so` to the true device which will be used at run-time (here the USRP one,`liboai_usrpdevif.so` . USRP is the only hardware tested today in the Continuous Integration process.
- `--eNB` is to build the `lte-softmodem` executable and all required shared libraries
- `--UE` is to build the `lte-uesoftmodem` executable and all required shared libraries

You can build the eNodeB and the UE separately, you may not need both of them depending on your oai usage.

After completing the build, the binaries are available in the `cmake_targets/lte_build_oai/build` directory. A copy is also available in the `target/bin` directory, with all binaries suffixed by the 3GPP release number, today .Rel14 by default. It must be noticed that the option for building for a specific 3GPP release number is not tested by the CI and may be removed in the future. 

# Building Optional Binaries

## Telnet Server

The telnet server can be built  with the --build-telnet option, after building the softmodem or while building it.

`./build_oai -I -w USRP --eNB --UE --build-telnetsrv`

or

`./build_oai  --build-telnetsrv`

## USRP record player

The USRP record player today needs a specific build. Work to make it available as a run time option is under consideration

# `build_oai` options

| Option                                                      | Status                                      | Description                                                  |
| ----------------------------------------------------------- | ------------------------------------------- | :----------------------------------------------------------- |
| -h                                                          | maintained                                  | get help                                                     |
| -c                                                          | maintained                                  | erase all previously built files for this target before starting the new build. |
| -C                                                          | maintained, needs improvement               | erase all previously built files for any target before starting the new build. |
| --verbose-compile                                           | maintained                                  |                                                              |
| --cflags_processor                                          | maintained                                  | used to pass options to the compiler                         |
| --clean-kernel                                              | unknown                                     | no code in the script corresponding to this option           |
| --install-system-files                                      | maintained                                  | install oai built binaries in linux system files repositories |
| -w                                                          | maintained and tested in CI for USRP device | build corresponding oai device and create the soft link to enforce this device usage at run-time |
| -t                                                          | maintained                                  | build the specified transport library, which is used in some simulators and in non monolithic eNodeB deployments. Now of little interest as transport library build is enforced when building the eNodeB. |
| --phy_simulators                                            | maintained, tested in CI                    | build all PHY simulators, a set of executables allowing unitary tests of LTE channel implementation within oai. |
| --core_simulators                                           |                                             |                                                              |
| -s                                                          |                                             |                                                              |
| --run-group                                                 |                                             |                                                              |
| -I                                                          | maintained, tested in CI                    | install external dependencies before starting the build      |
| --install-optional-packages                                 | maintained                                  | install optional packages, useful for developing and testing. look at the |
|                                                             |                                             | check_install_additional_tools function in cmake_targets/tools/build_helper script to get the list |
| -g                                                          | maintained                                  | build binaries with gdb support.                             |
| --eNB                                                       | maintained and tested in CI                 | build `lte-softmodem` the LTE eNodeB                         |
| --UE                                                        | maintained and tested in CI                 | build `lte-uesoftmodem` the LTE UE                           |
| --usrp-recplay                                              | maintained                                  | build with support for the record player. Implementation will be soon reviewed to switch to a run-time option. |
| --build-telnet                                              | maintained                                  | build the telnet server shared library, which can then be loaded at run time via the --telnetsrv command line option. |
| --build-msc                                                 | unknown                                     | build the msc shared library, which can then be loaded at run time via the --msc command line option. msc is a tracing utility which status is unknown. |
| --UE-conf-nvram                                             |                                             |                                                              |
| --UE-gen-nvram                                              |                                             |                                                              |
| -r                                                          | unknown, to be removed                      | specifies which 3GPP release to build for. Only the default (today rel14) is tested in CI and it is likely that future oai release will remove this option |
| -V                                                          | deprecated                                  | Used to build with support for synchronization diagram utility. This is now available via the T-Tracer and is included if T-Tracer is not disabled. |
| -x                                                          | deprecated                                  | Used to build with support for embedded signal analyzer. This is now available via the T-Tracer and is included if T-Tracer is not disabled. |
| --noS1                                                      | deprecated, to be removed                   | build noS1 version of oai softmodem binaries. noS1 allows running oai eNodeB and UE without an LTE core network (EPC). This functionality is now available via a run-time option. |
| --build-doxygen                                             | unknown                                     | build doxygen documentation, many oai source files do not include doxygen comments |
| --disable-deadline --enable-deadline --disable-cpu-affinity | deprecated                                  | These options were used to activate or de-activate specific code depending on the choice of a specific linux scheduling  mode. This has not been tested for a while and should be implemented as configuration options |
| --disable-T-Tracer                                          | maintained, to be tested                    | Remove T_Tracer and console LOG messages except error messages. |
| --disable-hardware-dependency                               |                                             |                                                              |
| --ue-autotest-trace --ue-timing --ue-trace                  | deprecated                                  | Were used to enable conditional code implementing debugging messages or debugging statistics. These functionalities are now either available from run-time options or not maintained. |
| --disable-log                                               | deprecated, to be removed                   | Was used to disable LOG messages, replaced by `--disable-T-Tracer` build option to remove tracing code from the build or `--T-stdout 0` run time option to redirect LOG messages to T-Tracer client interface. |
| --ue-nas-use-tun                                            | deprecated to be removed                    | Usage of tun  in place of specific kernel modules is now a run time option. `--nokrnmod 0` option disable the default behavior which is to use tun to send or receive ip packets to/from the linux ip stack. |
| --build-eclipse                                             | unknown                                     |                                                              |
| --basic-simulator                                           | deprecated tested in CI                     | builds the basic simulator device. Now of little interest as this device build is enforced when building the eNodeB and it can be used at run time with the `--basicsim` option. More over the rf simulator is an enhanced basic simulator. |
| --rfsimulator                                               | maintained                                  | builds the rf simulator device. Now of little interest as this device build is enforced when building the eNodeB and it can be used at run time with the `--rfsim` option. |
|                                                             |                                             |                                                              |

[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[running the oai softmodem ](RUNMODEM.md)

