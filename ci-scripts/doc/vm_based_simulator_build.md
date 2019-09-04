<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: build an OAI variant</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Detailed Description](#2-detailed-description)
3.  [Typical Usage](#3-typical-usage)

# 1. Introduction #

```bash
$ cd /tmp/CI-raphael
$ ls *.zip
localZip.zip
$ ./ci-scripts/oai-ci-vm-tool build --help
OAI CI VM script
   Original Author: Raphael Defosseux
   Requirements:
     -- uvtool uvtool-libvirt apt-cacher
     -- xenial image already synced
   Default:
     -- eNB with USRP

Usage:
------
    oai-ci-vm-tool build [OPTIONS]

Mandatory Options:
--------
    --job-name #### OR -jn ####
    Specify the name of the Jenkins job.

    --build-id #### OR -id ####
    Specify the build ID of the Jenkins job.

    --workspace #### OR -ws ####
    Specify the workspace.

Options:
--------
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

    --daemon OR -D
    Run as daemon

    --help OR -h
    Print this help message.
```

# 2. Detailed Description #

Source file concerned: `ci-scripts/buildOnVM.sh`

## 2.1. build_on_vm function ##

*  First check if the ZIP file is available and if the apt-cacher proxy configuration file is present. If not, it will stop.
*  Check if the VM instance is alive. If not, create it.
*  Once the VM is alive, retrieve the IP address with VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
*  We copy the ZIP file to the VM : `scp localZip.zip ubuntu@$VM_IP_ADDR:/home/ubuntu`
*  apt-cacher proxy file: `scp etc/apt/apt.conf.d/01proxy ubuntu@$VM_IP_ADDR:/home/ubuntu`

Then we open a SSH session on the VM : `ssh ubuntu@$VM_IP_ADDR`

*  We copy the apt-cacher proxy file to its destination space: `sudo cp 01proxy /etc/apt/apt.conf.d/`
*  We create an hush login file to eliminate the ssh opening session messages.
*  We unzip the ZIP file into `/home/ubuntu/tmp/` folder
*  and we work from there.

# 3. Typical Usage #

## 3.1. Build in the foreground, check the results and destroy the VM at the end ##

```bash
$ cd /tmp/CI-raphael
$ ls *.zip
localZip.zip
$ ./ci-scripts/oai-ci-vm-tool build --workspace /tmp/CI-raphael --job-name RAN-CI-develop --build-id 47 --variant enb-usrp
############################################################
OAI CI VM script
############################################################
VM_NAME             = RAN-CI-develop-b47-enb-usrp
VM_CMD_FILE         = RAN-CI-develop-b47-enb-usrp_cmds.txt
JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/enb_usrp
BUILD_OPTIONS       = --eNB -w USRP --mu
VM_MEMORY           = 2048 MBytes
VM_CPU              = 4
############################################################
Creating VM (RAN-CI-develop-b47-enb-usrp) on Ubuntu Cloud Image base
############################################################
Waiting for VM to be started
Warning: Permanently added '192.168.122.2' (ECDSA) to the list of known hosts.
RAN-CI-develop-b47-enb-usrp has for IP addr = 192.168.122.2
############################################################
Copying GIT repo into VM (RAN-CI-develop-b47-enb-usrp)
############################################################
Warning: Permanently added '192.168.122.2' (ECDSA) to the list of known hosts.
############################################################
Running install and build script on VM (RAN-CI-develop-b47-enb-usrp)
############################################################
Welcome to Ubuntu 16.04.6 LTS (GNU/Linux 4.4.0-145-generic x86_64)

 * Documentation:  https://help.ubuntu.com
 * Management:     https://landscape.canonical.com
 * Support:        https://ubuntu.com/advantage

  Get cloud support with Ubuntu Advantage Cloud Guest:
    http://www.ubuntu.com/business/services/cloud

0 packages can be updated.
0 updates are security updates.

New release '18.04.2 LTS' available.
Run 'do-release-upgrade' to upgrade to it.


sudo apt-get --yes --quiet install zip subversion libboost-dev 
unzip -qq -DD ../localZip.zip
cd /home/ubuntu/tmp
source oaienv
cd cmake_targets
./build_oai -I --eNB -w USRP --mu

#  Here wait for a few minutes 8 to 12 minutes #

############################################################
Creating a tmp folder to store results and artifacts
############################################################
############################################################
Destroying VM
############################################################
# Host 192.168.122.2 found: line 19
/home/eurecom/.ssh/known_hosts updated.
Original contents retained as /home/eurecom/.ssh/known_hosts.old
############################################################
Checking build status
############################################################
STATUS seems OK
```

If you are adding the `-k` or `--keep-vm-alive` option, the VM instance will not be destroyed and you explore what happenned.

## 3.2. Build in the background ##

This is how it is done in the CI master job pipeline.

```bash
$ cd /tmp/CI-raphael
$ ls *.zip
localZip.zip
$ ./ci-scripts/oai-ci-vm-tool build --workspace /tmp/CI-raphael --job-name RAN-CI-develop --build-id 47 --variant phy-sim --daemon
12:44:24  ############################################################
12:44:24  OAI CI VM script
12:44:24  ############################################################
12:44:24  VM_NAME             = RAN-CI-develop-b47-phy-sim
12:44:24  VM_CMD_FILE         = RAN-CI-develop-b47-phy-sim_cmds.txt
12:44:24  JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
12:44:24  ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/phy_sim
12:44:24  BUILD_OPTIONS       = --phy_simulators
12:44:24  VM_MEMORY           = 2048 MBytes
12:44:24  VM_CPU              = 4
12:44:24  ############################################################
12:44:24  Creating VM (RAN-CI-develop-b47-phy-sim) on Ubuntu Cloud Image base
12:44:24  ############################################################
12:44:27  Waiting for VM to be started
12:46:34  Warning: Permanently added '192.168.122.220' (ECDSA) to the list of known hosts.
12:46:34  RAN-CI-develop-b47-phy-sim has for IP addr = 192.168.122.220
12:46:34  ############################################################
12:46:34  Copying GIT repo into VM (RAN-CI-develop-b47-phy-sim)
12:46:34  ############################################################
12:46:34  Warning: Permanently added '192.168.122.220' (ECDSA) to the list of known hosts.
12:46:34  ############################################################
12:46:34  Running install and build script on VM (RAN-CI-develop-b47-phy-sim)
12:46:34  ############################################################
12:46:34  Welcome to Ubuntu 16.04.6 LTS (GNU/Linux 4.4.0-145-generic x86_64)
12:46:34  
12:46:34   * Documentation:  https://help.ubuntu.com
12:46:34   * Management:     https://landscape.canonical.com
12:46:34   * Support:        https://ubuntu.com/advantage
12:46:34  
12:46:34    Get cloud support with Ubuntu Advantage Cloud Guest:
12:46:34      http://www.ubuntu.com/business/services/cloud
12:46:34  
12:46:34  0 packages can be updated.
12:46:34  0 updates are security updates.
12:46:34  
12:46:34  New release '18.04.2 LTS' available.
12:46:34  Run 'do-release-upgrade' to upgrade to it.
12:46:34  
12:46:34  
12:46:34  sudo apt-get --yes --quiet install zip daemon subversion libboost-dev 
12:46:46  unzip -qq -DD ../localZip.zip
12:46:48  source oaienv
12:46:48  sudo -E daemon --inherit --unsafe --name=build_daemon --chdir=/home/ubuntu/tmp/cmake_targets -o /home/ubuntu/tmp/cmake_targets/log/install-build.txt ./my-vm-build.sh
12:46:48  STATUS seems OK
```

So here is 2.5 minutes, a VM was created, source files copied and the build process is started in the background.

```bash
$ cd /tmp/raphael
$ ./ci-scripts/oai-ci-vm-tool wait --workspace /var/jenkins/workspace/RAN-CI-develop --variant phy-sim --job-name RAN-CI-develop --build-id 47 --keep-vm-alive
12:49:14  ############################################################
12:49:14  OAI CI VM script
12:49:14  ############################################################
12:49:14  VM_NAME             = RAN-CI-develop-b47-phy-sim
12:49:14  VM_CMD_FILE         = RAN-CI-develop-b47-phy-sim_cmds.txt
12:49:14  JENKINS_WKSP        = /var/jenkins/workspace/RAN-CI-develop
12:49:14  ARCHIVES_LOC        = /var/jenkins/workspace/RAN-CI-develop/archives/phy_sim
12:49:14  BUILD_OPTIONS       = --phy_simulators
12:49:15  Waiting for VM to be started
12:49:15  Warning: Permanently added '192.168.122.220' (ECDSA) to the list of known hosts.
12:49:16  RAN-CI-develop-b47-phy-sim has for IP addr = 192.168.122.220
12:49:16  ############################################################
12:49:16  Waiting build process to end on VM (RAN-CI-develop-b47-phy-sim)
12:49:16  ############################################################
12:49:16  ps -aux | grep build 
12:54:23  ############################################################
12:54:23  Creating a tmp folder to store results and artifacts
12:54:23  ############################################################
12:54:23  ############################################################
12:54:23  Checking build status
12:54:23  ############################################################
12:54:23  STATUS seems OK
```

Here the `--keep-vm-alive` option is used to keep the VM alive and performs some testing.

---

Next step: [how the build is checked](./vm_based_simulator_check_build.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

