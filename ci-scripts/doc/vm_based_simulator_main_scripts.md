<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: the Main Script</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Sub BASH scripts](#2-sub-bash-scripts)
3.  [Main script features](#3-main-script-features)

# 1. Introduction #

The file name is `./ci-scripts/oai-ci-vm-tool` from the workspace root.

```bash
$ cd /tmp/CI-raphael
$ ./ci-scripts/oai-ci-vm-tool --help
OAI CI VM script
   Original Author: Raphael Defosseux
   Requirements:
     -- uvtool uvtool-libvirt apt-cacher
     -- xenial image already synced

Usage:
------
    oai-ci-vm-tool (-h|--help) {create,destroy,build,wait,test,report-build,report-test} ...

```

This is a **BASH** script.


# 2. Sub BASH scripts #

The main script is including a bunch of sub BASH scripts.

*  ci-scripts/createVM.sh
*  ci-scripts/buildOnVM.sh
*  ci-scripts/waitBuildOnVM.sh
*  ci-scripts/destroyAllRunningVM.sh
*  ci-scripts/runTestOnVM.sh
*  ci-scripts/reportBuildLocally.sh
*  ci-scripts/reportTestLocally.sh

**NOTE: ci-scripts/runTestOnVM.sh is getting big and will certainly be split to facilitate maintenance. Start functions will be also factorized.**

# 3. Main script features #

The main purpose of the main script is decipher the options and launch the requested function.

It is also **testing if uvtool and apt-cacher are installed.**

It finally provides parameters to the requested functions. Parameter definition is centralized there.

For example:

for VM instance creation:

*  the instance name:  VM_NAME
*  the RAM and number of CPUs: VM_MEMORY, VM_CPU

for OAI variant build:

*  build options: BUILD_OPTIONS
*  build log file to parse: LOG_PATTERN
*  the number of log files to parse: NB_PATTERN_FILES

You can get the list of variant specific variables specifying the variant argument when asking for help:

``` bash
./ci-scripts/oai-ci-vm-tool help variant
    --variant flexran-rtc     OR -v10    ( build and test non-OSA )
               VM_NAME=ci-flexran-rtc           ARCHIVES_LOC=flexran        
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=1               BUILD_OPTIONS="cmake . && make -j2"
               LOG_PATTERN=.Rel14.txt     

    --variant enb-usrp        OR -v1     ( build and test  )
               VM_NAME=ci-enb-usrp              ARCHIVES_LOC=enb_usrp       
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=9               BUILD_OPTIONS="--eNB -w USRP --mu"
               LOG_PATTERN=.Rel14.txt     

    --variant l1-sim          OR -v20    ( test  )
               VM_NAME=ci-l1-sim                ARCHIVES_LOC=l1_sim         
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=9               BUILD_OPTIONS=""             
               LOG_PATTERN=.Rel14.txt     

    --variant rf-sim          OR -v21    ( test  )
               VM_NAME=ci-rf-sim                ARCHIVES_LOC=rf_sim         
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=9               BUILD_OPTIONS=""             
               LOG_PATTERN=.Rel14.txt     

    --variant l2-sim          OR -v22    ( test  )
               VM_NAME=ci-l2-sim                ARCHIVES_LOC=l2_sim         
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=9               BUILD_OPTIONS=""             
               LOG_PATTERN=.Rel14.txt     

    --variant basic-sim       OR -v2     ( build and test  )
               VM_NAME=ci-basic-sim             ARCHIVES_LOC=basic_sim      
               VM_MEMORY=8192                   VM_CPU=4              
               NB_PATTERN_FILES=13              BUILD_OPTIONS="--eNB --UE"   
               LOG_PATTERN=.Rel14.txt     

    --variant phy-sim         OR -v3     ( build and test  )
               VM_NAME=ci-phy-sim               ARCHIVES_LOC=phy_sim        
               VM_MEMORY=2048                   VM_CPU=4              
               NB_PATTERN_FILES=3               BUILD_OPTIONS="--phy_simulators"
               LOG_PATTERN=.Rel14.txt     

    --variant cppcheck        OR -v4     ( build and test  )
               VM_NAME=ci-cppcheck              ARCHIVES_LOC=cppcheck       
               VM_MEMORY=4096                   VM_CPU=4              
               NB_PATTERN_FILES=1               BUILD_OPTIONS="--enable=warning --force --xml --xml-version=2 --suppressions-list=ci-scripts/cppcheck_suppressions.list -I common/utils -j4"
               LOG_PATTERN=cppcheck.xml   

    --variant enb-ethernet    OR -v7     ( build and test  )
               VM_NAME=ci-enb-ethernet          ARCHIVES_LOC=enb_eth        
               VM_MEMORY=4096                   VM_CPU=4              
               NB_PATTERN_FILES=8               BUILD_OPTIONS="--eNB"        
               LOG_PATTERN=.Rel14.txt     

    --variant ue-ethernet     OR -v8     ( build and test  )
               VM_NAME=ci-ue-ethernet           ARCHIVES_LOC=ue_eth         
               VM_MEMORY=4096                   VM_CPU=4              
               NB_PATTERN_FILES=12              BUILD_OPTIONS="--UE"         
               LOG_PATTERN=.Rel14.txt     

```

To define a new variant you just need to define a function which name conforms to `function variant__v<n>__<variant_name>` where n and variant_name will respectively define the short and long options for your variant. The function only needs to define the variant dependent variables. For many variables, default values are set in the `check_set_variant` function. When a variant doesn't define the BUILD_OPTIONS variable it cannot be used for the `build` `wait` and `create` commands.

The main scripts also allows the definition of non variant-dependant variable via the `--setvar_<variable name> <variable value>` options.
You can get the list of these variables by using `help setvar`:

```BASH
./ci-scripts/oai-ci-vm-tool help setvar
--setvar_<varname> <value> where varname is one of:
            VM_OSREL :     OS release to use in virtual machines
    RUN_EXPERIMENTAL :     Enforce execution of variants with EXPERIMENTAL variable set to "true"
```

To add a new non-variant dependant variable you need:
* Add an item to the `AUTHORIZED_VAR` array
* In the `setvar_usage`function,  add your help string in the HELP_VAR["<your variable name>"] variable.
* Write the bash code for your variable.

Example of non variant dependent usage:

``` bash
./ci-scripts/oai-ci-vm-tool test -v21  -ws /usr/local/oai/enhance_CI_extEPC/openairinterface5g -id 1 -jn testci
Currently testci-b1-rf-sim Testing is not implemented / enabled
Comment out these lines in ./ci-scripts/oai-ci-vm-tool if you want to run it
 or use option --setvar_RUN_EXPERIMENTAL=true to test it


 ./ci-scripts/oai-ci-vm-tool test -v21  -ws /usr/local/oai/enhance_CI_extEPC/openairinterface5g -id 1 -jn testci --setvar_RUN_EXPERIMENTAL true
 Setting RUN_EXPERIMENTAL to true...
 ############################################################
 OAI CI VM script
 ############################################################
 ENB_VM_NAME         = testci-b1-enb-ethernet
 ENB_VM_CMD_FILE     = testci-b1-enb-ethernet_cmds.txt
 UE_VM_NAME          = testci-b1-ue-ethernet
 UE_VM_CMD_FILE      = testci-b1-ue-ethernet_cmds.txt
 JENKINS_WKSP        = /usr/local/oai/enhance_CI_extEPC/openairinterface5g
 ARCHIVES_LOC        = /usr/local/oai/enhance_CI_extEPC/openairinterface5g/archives/rf_sim/test
 ############################################################
 Waiting for ENB VM to be started
...........................
```

In the same way, you can set the variable `VM_OSREL` to run the test in virtual machines of the specified OS release:

``` bash
./ci-scripts/oai-ci-vm-tool test -v21  -ws /usr/local/oai/enhance_CI_extEPC/openairinterface5g -id 1 -jn testci --setvar_VM_OSREL bionic
```

---

Next step: [how to create one or several VM instances](./vm_based_simulator_create.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)
