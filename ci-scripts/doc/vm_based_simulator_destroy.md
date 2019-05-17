<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Test Environment: Properly Destroy all VM instances</font></b>
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
$ ./ci-scripts/oai-ci-vm-tool destroy --help
OAI CI VM script
   Original Author: Raphael Defosseux
   Requirements:
     -- uvtool uvtool-libvirt apt-cacher

Usage:
------
    oai-ci-vm-tool destroy [OPTIONS]

Mandatory Options:
--------
    --job-name #### OR -jn ####
    Specify the name of the Jenkins job.

    --build-id #### OR -id ####
    Specify the build ID of the Jenkins job.

Options:
--------
    --help OR -h
    Print this help message.
```

# 2. Detailed Description #

Source file concerned: `ci-scripts/destroyAllRunningVM.sh`

## 2.1. destroy_vm function ##

This is the function that is being called from the main oai-vm-tool script.

The main purpose is to destroy all VM instances whose name matches a pattern.

It also cleans up the `.ssh/known_hosts` file.

# 3. Typical Usage #

```bash
$ cd /tmp/CI-raphael
$ ./ci-scripts/oai-ci-vm-tool destroy --job-name raphael --build-id 1
```

---

You can go back to the [CI dev main page](./ci_dev_home.md)

