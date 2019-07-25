<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: Testing an OAI variant</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Detailed Description](#2-detailed-description)
3.  [Typical Usage](#3-typical-usage)
    1.  [Testing the physical simulators](#31-testing-the-physicals-simulators)
    2.  [Testing the basic simulator](#32-testing-the-basic-simulator)
    3.  [Testing the RF simulator](#33-testing-the-rf-simulator)
    4.  [Testing the L2-nFAPI simulator](#33-testing-the-l2-nfapi-simulator)

# 1. Introduction #

Currently 2 build variants can be directly tested:

*  Physical Simulators
*  Basic Simulator

In addition, 2 build variants are used:

*  OAI eNB with ETHERNET transport
*  OAI UE with ETHERNET transport

for the following scenarios:

*  L1 simulator w/ a channel simulator (NOT IMPLEMENTED)
*  RF simulator : (IMPLEMENTED but not working as of 2019.w15)
*  L2 nFAPI simulator

Tests are run sequentially in the Jenkins pipeline because:

*  We want to mutualize the VM creation for an EPC
*  We have seen performance issues when running in parallel.


```bash
./ci-scripts/oai-ci-vm-tool test --help
OAI CI VM script
   Original Author: Raphael Defosseux
   Requirements:
     -- uvtool uvtool-libvirt apt-cacher
     -- xenial image already synced
   Default:
     -- eNB with USRP

Usage:
------
    oai-ci-vm-tool test [OPTIONS]

Options:
--------
    --job-name #### OR -jn ####
    Specify the name of the Jenkins job.

    --build-id #### OR -id ####
    Specify the build ID of the Jenkins job.

    --workspace #### OR -ws ####
    Specify the workspace.

 # OpenAirInterface Build Variants
    --variant enb-usrp     OR -v1
    --variant basic-sim    OR -v2
    --variant phy-sim      OR -v3
    --variant cppcheck     OR -v4
    --variant enb-ethernet OR -v7
    --variant ue-ethernet  OR -v8
 # non-OSA Build Variants
    --variant flexran-rtc  OR -v10
 # OpenAirInterface Test Variants
    --variant l1-sim       OR -v20
    --variant rf-sim       OR -v21
    --variant l2-sim       OR -v22
    Specify the variant to build.

    --keep-vm-alive OR -k
    Keep the VM alive after the build.

    --help OR -h
    Print this help message.
```

# 2. Detailed Description #

Source file concerned: `ci-scripts/run_test_on_vm.sh`

**TBD when file is re-structured.**

# 3. Typical Usage #

## 3.1. Testing the physical simulators ##

```bash
$ ./ci-scripts/oai-ci-vm-tool test --workspace /var/jenkins/workspace/RAN-CI-develop --variant phy-sim --job-name RAN-CI-develop --build-id 47
12:54:29  ############################################################
12:54:29  OAI CI VM script
12:54:29  ############################################################
12:54:29  VM_NAME             = RAN-CI-develop-b47-phy-sim
12:54:29  VM_CMD_FILE         = RAN-CI-develop-b47-phy-sim_cmds.txt
12:54:29  JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
12:54:29  ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/phy_sim/test
12:54:29  ############################################################
12:54:29  Waiting for VM to be started
12:54:29  ############################################################
12:54:29  Warning: Permanently added '192.168.122.220' (ECDSA) to the list of known hosts.
12:54:30  RAN-CI-develop-b47-phy-sim has for IP addr = 192.168.122.220
...
13:04:48  Test Results are written to /home/ubuntu/tmp/cmake_targets/autotests/log/results_autotests.xml
13:05:00  ############################################################
13:05:00  Creating a tmp folder to store results and artifacts
13:05:00  ############################################################
13:05:00  /var/jenkins/workspace/RAN-CI-develop/archives/phy_sim/test /var/jenkins/workspace/RAN-CI-develop
13:05:04  /var/jenkins/workspace/RAN-CI-develop
13:05:04  ############################################################
13:05:04  Destroying VM
13:05:04  ############################################################
13:05:06  # Host 192.168.122.220 found: line 21
13:05:06  /home/eurecom/.ssh/known_hosts updated.
13:05:06  Original contents retained as /home/eurecom/.ssh/known_hosts.old
13:05:06  ############################################################
13:05:06  Checking run status
13:05:06  ############################################################
13:05:06  NB_FOUND_FILES = 1
13:05:06  NB_RUNS        = 20
13:05:06  NB_FAILURES    = 0
13:05:06  STATUS seems OK
```

Note that the VM instance is destroyed. You do that when you are sure your test is passing.

## 3.2. Testing the basic simulator ##

```bash
$ ./ci-scripts/oai-ci-vm-tool test --workspace /var/jenkins/workspace/RAN-CI-develop --variant basic-sim --job-name RAN-CI-develop --build-id 48
15:11:13  ############################################################
15:11:13  OAI CI VM script
15:11:13  ############################################################
15:11:13  VM_NAME             = RAN-CI-develop-b48-basic-sim
15:11:13  VM_CMD_FILE         = RAN-CI-develop-b48-basic-sim_cmds.txt
15:11:13  JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
15:11:13  ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/basic_sim/test
15:11:13  ############################################################
15:11:13  Waiting for VM to be started
15:11:13  ############################################################
15:11:14  Warning: Permanently added '192.168.122.29' (ECDSA) to the list of known hosts.
15:11:15  RAN-CI-develop-b48-basic-sim has for IP addr = 192.168.122.29
15:11:15  ############################################################
15:11:15  Test EPC on VM (RAN-CI-develop-b48-epc) will be using ltebox
15:11:15  ############################################################
15:11:15  EPC_VM_CMD_FILE     = RAN-CI-develop-b48-epc_cmds.txt
15:11:15  ############################################################
15:11:15  Creating test EPC VM (RAN-CI-develop-b48-epc) on Ubuntu Cloud Image base
15:11:15  ############################################################
15:11:18  Waiting for VM to be started
15:13:25  Warning: Permanently added '192.168.122.156' (ECDSA) to the list of known hosts.
15:13:25  RAN-CI-develop-b48-epc has for IP addr = 192.168.122.156
15:13:25  Warning: Permanently added '192.168.122.156' (ECDSA) to the list of known hosts.
15:13:25  ls: cannot access '/opt/ltebox/tools/start_ltebox': No such file or directory
15:13:25  ############################################################
15:13:25  Copying ltebox archives into EPC VM (RAN-CI-develop-b48-epc)
15:13:25  ############################################################
15:13:25  ############################################################
15:13:25  Install EPC on EPC VM (RAN-CI-develop-b48-epc)
15:13:25  ############################################################
....
15:24:39  ############################################################
15:24:39  Terminate EPC
15:24:39  ############################################################
15:24:39  cd /opt/ltebox/tools
15:24:39  sudo ./stop_ltebox
15:24:40  sudo daemon --name=simulated_hss --stop
15:24:40  sudo killall --signal SIGKILL hss_sim
15:24:40  ############################################################
15:24:40  Destroying VMs
15:24:40  ############################################################
15:24:41  # Host 192.168.122.29 found: line 18
15:24:41  /home/eurecom/.ssh/known_hosts updated.
15:24:41  Original contents retained as /home/eurecom/.ssh/known_hosts.old
15:24:43  # Host 192.168.122.60 found: line 20
15:24:43  /home/eurecom/.ssh/known_hosts updated.
15:24:43  Original contents retained as /home/eurecom/.ssh/known_hosts.old
15:24:43  ############################################################
15:24:43  Checking run status
15:24:43  ############################################################
15:24:43  STATUS seems OK
```

## 3.3. Test the RF simulator ##

```bash
./ci-scripts/oai-ci-vm-tool test --workspace /var/jenkins/workspace/RAN-CI-develop --variant rf-sim --job-name RAN-CI-develop --build-id 48 --keep-vm-alive
15:24:45  Currently RF-Simulator Testing is not implemented / enabled
15:24:45  Comment out these lines in ./ci-scripts/oai-ci-vm-tool if you want to run it
15:24:45  STATUS seems OK
```

## 3.4. Testing the L2-nFAPI simulator

```bash
./ci-scripts/oai-ci-vm-tool test --workspace /var/jenkins/workspace/RAN-CI-develop --variant l2-sim --job-name RAN-CI-develop --build-id 48
15:24:47  ############################################################
15:24:47  OAI CI VM script
15:24:47  ############################################################
15:24:47  ENB_VM_NAME         = RAN-CI-develop-b48-enb-ethernet
15:24:47  ENB_VM_CMD_FILE     = RAN-CI-develop-b48-enb-ethernet_cmds.txt
15:24:47  UE_VM_NAME          = RAN-CI-develop-b48-ue-ethernet
15:24:47  UE_VM_CMD_FILE      = RAN-CI-develop-b48-ue-ethernet_cmds.txt
15:24:47  JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
15:24:47  ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/l2_sim/test
15:24:47  ############################################################
15:24:47  Waiting for ENB VM to be started
15:24:47  ############################################################
15:24:47  Warning: Permanently added '192.168.122.110' (ECDSA) to the list of known hosts.
15:24:48  RAN-CI-develop-b48-enb-ethernet has for IP addr = 192.168.122.110
15:24:48  ############################################################
15:24:48  Waiting for UE VM to be started
15:24:48  ############################################################
15:24:49  Warning: Permanently added '192.168.122.90' (ECDSA) to the list of known hosts.
15:24:50  RAN-CI-develop-b48-ue-ethernet has for IP addr = 192.168.122.90
15:24:50  ############################################################
15:24:50  Test EPC on VM (RAN-CI-develop-b48-epc) will be using ltebox
15:24:50  ############################################################
15:24:50  EPC_VM_CMD_FILE     = RAN-CI-develop-b48-epc_cmds.txt
15:24:50  Waiting for VM to be started
15:24:50  Warning: Permanently added '192.168.122.156' (ECDSA) to the list of known hosts.
15:24:51  RAN-CI-develop-b48-epc has for IP addr = 192.168.122.156
...
15:42:44  ############################################################
15:42:44  Destroying VMs
15:42:44  ############################################################
15:42:46  # Host 192.168.122.110 found: line 18
15:42:46  /home/eurecom/.ssh/known_hosts updated.
15:42:46  Original contents retained as /home/eurecom/.ssh/known_hosts.old
15:42:47  # Host 192.168.122.90 found: line 18
15:42:47  /home/eurecom/.ssh/known_hosts updated.
15:42:47  Original contents retained as /home/eurecom/.ssh/known_hosts.old
15:42:47  ############################################################
15:42:47  Checking run status
15:42:47  ############################################################
15:42:47  STATUS failed?
```
---

Final step: [how to properly destroy all VM instances](./vm_based_simulator_destroy.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

