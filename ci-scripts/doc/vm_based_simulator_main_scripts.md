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

**NOTE: ci-scripts/runTestOnVM.sh is getting big and will certainly be split to facilate maintenance. Start functions will be also factorized.**

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

These last 2 variables are very important if you change the build options or if you modify the build system and add more targets to build (especially true for physical simulator).

There are many more variables.

---

Next step: [how to create one or several VM instances](./vm_based_simulator_create.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

