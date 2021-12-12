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

The OAI EPC is developed in a distinct project with it's own [documentation](https://github.com/OPENAIRINTERFACE/openair-epc-fed/wiki) , it is not described here.

OAI softmodem sources, which aim to implement 3GPP compliant UEs, eNodeB and gNodeB can be downloaded from the Eurecom [gitlab repository](./GET_SOURCES.md).

Sources come with a build script [build_oai](../cmake_targets/build_oai) located at the root of the `openairinterface5g/cmake_targets` directory. This script is developed to build the oai binaries (executables,shared libraries) for different hardware platforms, and use cases. 

The main oai binaries, which are tested by the Continuous Integration process are:

-  The LTE UE: `lte-uesoftmodem`
-  The 5G UE: `nr-uesoftmodem`
-  The LTE eNodeB: `lte-softmodem`
-  The 5G gNodeB: `nr-softmodem`
-  The LTE PHY simulators: `dlsim` and `ulsim`
-  The 5G PHY simulators: `nr_dlschsim` `nr_dlsim`   `nr_pbchsim` `nr_pucchsim` `nr_ulschsim` `nr_ulsim` `polartest` `smallblocktest`
   `ulsim` `ldpctest`

Running the  [build_oai](../cmake_targets/build_oai) script also generates some utilities required to build and/or run the oai softmodem binaries:

- `conf2uedata`: a binary used to build the UE data from a configuration file. The created file emulates the sim card  of a 3GPP compliant phone.
- `nvram`: a binary used to build UE (IMEI...) and EMM (IMSI, registered PLMN) non volatile data. 
- `rb_tool`: radio bearer utility 
- `genids` T Tracer utility, used at build time to generate T_IDs.h include file. This binary is located in the [T Tracer source file directory](../common/utils/T) .

The build system for OAI uses [cmake](https://cmake.org/) which is a  tool to generate makefiles. The `build_oai` script is a wrapper using cmake, make and standard linux shell commands to ease the oai build and use . The file describing how to build the executables from source files is the [CMakeLists.txt](../cmake_targets/CMakeLists.txt), it is used as input by cmake to generate the makefiles.

The oai softmodem supports many use cases, and new ones are regularly added. Most of them are accessible using the configuration file or the command line options and continuous effort is done to avoid introducing build options as it makes tests and usage more complicated than run-time options. The following functionalities, originally requiring a specific build are now accessible by configuration or command line options:

- s1, noS1
- all simulators as the rfsimulator, the L2 simulator, with exception of PHY simulators, which are distinct executables. 


Calling the `build_oai` script with the -h option gives the list of all available options, but a process to simplify and check the requirements of all these options is on-going. Check the [table](BUILD.md "# `build_oai` options") At the end of this page to know the status of `buid_oai` options which are not described hereafter.

# Building PHY Simulators

The PHY layer simulators (LTE and NR) can be built as follows:  

```
cd <your oai installation directory>/openairinterface5g/
source oaienv
cd cmake_targets/
./build_oai -I --phy_simulators
```

After completing the build, the binaries are available in the cmake_targets/phy_simulators/build directory.  
A copy is also available in the target/bin directory, with all binaries suffixed by the 3GPP release number, today **.Rel15**.  


Detailed information about these simulators can be found [in this dedicated page](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/OpenAirLTEPhySimul)

# Building UEs, eNodeB and gNodeB Executables

After downloading the source files, a single build command can be used to get the binaries supporting all the oai softmodem use cases (UE and [eg]NodeB):

```
cd <your oai installation directory>/openairinterface5g/
source oaienv
cd cmake_targets/
./build_oai -I -w USRP --eNB --UE --nrUE --gNB
```

- The `-I` option is to install pre-requisites, you only need it the first time you build the softmodem or when some oai dependencies have changed. Note: for Ubuntu 20 use cmake_targets/install_external_packages.ubuntu20 instead!
- The `-w` option is to select the radio head support you want to include in your build. Radio head support is provided via a shared library, which is called the "oai device" The build script creates a soft link from `liboai_device.so` to the true device which will be used at run-time (here the USRP one,`liboai_usrpdevif.so` . USRP is the only hardware tested today in the Continuous Integration process. The RF simulator[RF simulator](../targets/ARCH/rfsimulator/README.md) is implemented as a specific device replacing RF hardware, it can be specifically built using `-w SIMU` option, but is also built during any softmodem build.
- `--eNB` is to build the `lte-softmodem` executable and all required shared libraries
- `--gNB` is to build the `nr-softmodem` executable and all required shared libraries
- `--UE` is to build the `lte-uesoftmodem` executable and all required shared libraries
- `--nrUE` is to build the `nr-uesoftmodem` executable and all required shared libraries

You can build any oai softmodem executable separately, you may not need all of them depending on your oai usage.

After completing the build, the binaries are available in the `cmake_targets/ran_build/build` directory. A copy is also available in the `target/bin` directory, with all binaries suffixed by the 3GPP release number, today .Rel15.

When installing the pre-requisites, especially the `UHD` driver, you can now specify if you want to install from source or not.

- For `fedora`-based OS, it was already the case all the time. But now you can specify which version to install.
- For `ubuntu` OS, the Ettus PPA currently installs the following versions:
  * `Ubuntu16.04`: --> version `3.15.0.0`
  * `Ubuntu18.04`: --> version `4.1.0.0`

```bash
export BUILD_UHD_FROM_SOURCE=True
export UHD_VERSION=3.15.0.0
./build_oai -I -w USRP
```

The `UHD_VERSION` env variable `SHALL` be a valid tag (minus `v`) from the `https://github.com/EttusResearch/uhd.git` repository.

## Issue when building `nasmeh` module ##

A lot of users and contributors have faced the issue: `nasmesh` module does not build.

The reason is that the linux headers are not properly installed. For example:

```bash
$ uname -r
4.4.0-145-lowlatency
$ dpkg --list | grep 4.4.0-145-lowlatency | grep headers
ii  linux-headers-4.4.0-145-lowlatency         4.4.0-145.171
```

In my example it is properly installed.

Check on your environment:

```bash
$ uname -r
your-version
$ dpkg --list | grep your-version | grep headers
$
```

Install it:

```bash
$ sudo apt-get install --yes linux-headers-your-version
```

On CentOS or RedHat Entreprise Linux:

```bash
$ uname -r
3.10.0-1062.9.1.rt56.1033.el7.x86_64
$ yum list installed | grep kernel | grep devel
kernel-devel.x86_64              3.10.0-1062.9.1.el7         @rhel-7-server-rpms
kernel-rt-devel.x86_64           3.10.0-1062.9.1.rt56.1033.el7
```

If your kernel is generic, install `kernel-devel` package: `sudo yum install kernel-devel`

In most case, your kernel is real-time. Install `kernel-rt-devel` package: `sudo yum install kernel-rt-devel`

# Building Optional Binaries

## Telnet Server

The telnet server can be built  with the `--build-lib telnetsrv` option, after building the softmodem or while building it.

`./build_oai -I -w USRP --eNB --UE --build-lib telnetsrv`

or

`./build_oai  --build-lib telnetsrv`

You can get documentation about the telnet server  [here](common/utils/telnetsrv/DOC/telnetsrv.md)



## Other optional libraries

Using the help option of the build script you can get the list of available optional libraries. Those which haven't been mentioned above are known to need more tests and documentation. 

`./build_oai  --build-lib all` will build all available optional libraries.

# `build_oai` options

| Option                                                      | Status                                      | Description                                                  |
| ----------------------------------------------------------- | ------------------------------------------- | :----------------------------------------------------------- |
| -h                                                          | maintained                                  | get help                                                     |
| -c                                                          | maintained                                  | erase all previously built files for this target before starting the new build. |
| -C                                                          | maintained, needs improvement               | erase all previously built files for any target before starting the new build. |
| --verbose-compile                                           | maintained                                  | get compilation messages, as when running `make` or `gcc` directly. |
| --cflags_processor                                          | maintained                                  | used to pass options to the compiler.                        |
| --clean-kernel                                              | unknown                                     | no code in the script corresponding to this option           |
| --install-system-files                                      | maintained                                  | install oai built binaries in linux system files repositories |
| -w                                                          | maintained and tested in CI for USRP device | build corresponding oai device and create the soft link to enforce this device usage at run-time |
| --phy_simulators                                            | maintained, tested in CI                    | build all PHY simulators, a set of executables allowing unitary tests of LTE and 5G channel implementation within oai. |
| --core_simulators                                           |                                             |                                                              |
| -s                                                          |                                             |                                                              |
| --run-group                                                 |                                             |                                                              |
| -I                                                          | maintained, tested in CI                    | install external dependencies before starting the build      |
| --install-optional-packages                                 | maintained                                  | install optional packages, useful for developing and testing. look at the check_install_additional_tools function in cmake_targets/tools/build_helper script to get the list |
| -g                                                          | maintained                                  | Specifies the level of debugging options used to build the binaries. Available levels are `Release`, `RelWithDebInfo`, `MinSizeRe` and `Debug`. If -g is not specified, `Release` is used, if -g is used without any level, `Debug` is used. |
| -G                                                          | maintained                                  | Display Cmake debugging messages                             |
| --eNB                                                       | maintained and tested in CI                 | build `lte-softmodem` the LTE eNodeB                         |
| --UE                                                        | maintained and tested in CI                 | build `lte-uesoftmodem` the LTE UE                           |
| --gNB                                                       | maintained and tested in CI                 | build `nr-softmodem` the 5G gNodeB                           |
| --nrUE                                                      | maintained and tested in CI                 | build `nr-uesoftmodem` the 5G UE                             |
| --usrp-recplay                                              | deprecated                                  | use the USRP configuration parameters to use the record player. |
| --build-lib                                                 | maintained                                  | build  optional shared library(ies), which can then be loaded at run time via command line option. Use the --help option to get the list of supported optional libraries. `all` can be used to build all available optional libraries. |
| --UE-conf-nvram                                             | maintained                                  | Specifies the path to the input file used by the conf2uedata utility. defaults to [openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf](../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf) |
| --UE-gen-nvram                                              | maintained                                  | Specifies the path where the output file created by the conf2uedata utility will be placed. Defaults to `target/bin` |
| -V                                                          | deprecated                                  | Used to build with support for synchronization diagram utility. This is now available via the T-Tracer and is included if T-Tracer is not disabled. |
| --build-doxygen                                             | unknown                                     | build doxygen documentation, many oai source files do not include doxygen comments |
| --disable-deadline --enable-deadline --disable-cpu-affinity | deprecated                                  | These options were used to activate or de-activate specific code depending on the choice of a specific linux scheduling  mode. This has not been tested for a while and should be implemented as configuration options |
| --disable-T-Tracer                                          | maintained, to be tested                    | Remove T_Tracer and console LOG messages except error messages. |
| --ue-autotest-trace --ue-timing --ue-trace                  | deprecated                                  | Were used to enable conditional code implementing debugging messages or debugging statistics. These functionalities are now either available from run-time options or not maintained. |
| --build-eclipse                                             | unknown                                     |                                                              |
|                                                             |                                             |                                                              |

[oai wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[oai softmodem features](FEATURE_SET.md)

[running the oai softmodem ](RUNMODEM.md)

