<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: create a VM instance</font></b>
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
$ ./ci-scripts/oai-ci-vm-tool create --help
OAI CI VM script
   Original Author: Raphael Defosseux
   Requirements:
     -- uvtool uvtool-libvirt apt-cacher
     -- xenial image already synced
   Default:
     -- eNB with USRP

Usage:
------
    oai-ci-vm-tool create [OPTIONS]

Mandatory Options:
--------
    --job-name #### OR -jn ####
    Specify the name of the Jenkins job.

    --build-id #### OR -id ####
    Specify the build ID of the Jenkins job.

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

    --help OR -h
    Print this help message.
```

# 2. Detailed Description #

Source file concerned: `ci-scripts/createVM.sh`

## 2.1. create_vm function ##

This is the function that is being called from the main oai-vm-tool script.

The main purpose is to start a VM instance:

```bash
uvt-kvm create $VM_NAME release=xenial --memory $VM_MEMORY --cpu $VM_CPU --unsafe-caching --template ci-scripts/template-host.xml
```

Variables are set in the main script based on the options.

`--unsafe-caching` option is used because our VM instances are throw-away's. 

`--template ci-scripts/template-host.xml` is used to duplicate the CPU properties to the VM instance. **VERY IMPORTANT to build OAI**

## 2.2. Lock / Unlock functions ##

There are `acquire_vm_create_lock` and `release_vm_create_lock` functions.

Creating Virtual Machines instances in parallel **creates a lot of stress** on the host server HW. If you launch creations in parallel (Jenkins pipeline could do it) or you are several people working on the same host server, this mechanism atomizes the creation process and wait until the previous VM creation is finished.

# 3. Typical Usage #

```bash
$ cd /tmp/CI-raphael
$ ./ci-scripts/oai-ci-vm-tool create --job-name raphael --build-id 1 --variant phy-sim
# or a more **unique approach**
$ ./ci-scripts/oai-ci-vm-tool create -jn toto -id 1 -v2
```

The Jenkins pipeline uses the master job name as `job-name` option and the job-build ID.

Try to be unique if you are several developers working on the same host server.

Finally, typically I never use the `create` command. I use directly the build command that checks if VM is created and if not, will create it. See next step.

---

Next step: [how to build an OAI variant](./vm_based_simulator_build.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

