<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI CI Virtual-Machine-based Simulator Test Environment</font></b>
    </td>
  </tr>
</table>

## Table of Contents ##

1.  [Introduction](#1-introduction)
2.  [Prerequisites](#2-prerequisites)
    1.  [uvtool installation](#21-uvtool-installation)
    2.  [apt-cacher-server installation](#22-apt-cacher-server-installation)

# 1. Introduction #

This document explains how the master pipeline works and how any developer could contribute to add testing.

It is an extension to the wiki [page](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/ci/enb-master-job).

The purpose of this master pipeline is to:

*  Validate that a Merge Request is mergeable
*  Validate that a Merge Request is following coding guidelines
*  Validate that a Merge Request is not breaking any typical build variant
*  Validate that a Merge Request is not breaking any legacy simulator-based test

We will mainly focused on the 2 last items.

Last point, this documentation is valid for all CI-supported branches:

*  `master`
*  `develop`
*  `develop-nr`

But the feature set may not be aligned. **The principles still apply.**

# 2. Prerequisites #

Some details are available on this wiki [section](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/ci/enb-ci-architecture#22-pipeline-executor).

Currently we have a host server (`bellatrix`) with the current parameters:

*  40 x Intel(R) Xeon(R) CPU E5-2640 v4 @ 2.40GHz
*  64 Gbytes RAM
*  Ubuntu 16.04.3 LTS (xenial)

For you to replicate this environment, you need a strong server:

*  At least 16 cores
*  Ubuntu 16 (xenial) or higher (such as bionic, not tested)

Also we are using a Virtual Machine (VM for the rest of document) based strategy for the moment.

So you need to install 2 tools:

*  uvt-kvm
*  apt-cacher

We are planning to also add:

*  A Red Hat Linux Entreprise 7.6 host environment
*  A Container-based strategy (such as Docket and Kubernetes)

## 2.1. uvtool installation ##

```bash
$ sudo apt-get install uvtool
# if you don't have already, create an ssh key
$ ssh-keygen -b 2048
# retrieve an image
$ sudo uvt-simplestreams-libvirt sync arch=amd64 release=xenial
# we might soon switch to an Ubuntu 18.04 version
$ sudo uvt-simplestreams-libvirt sync arch=amd64 release=bionic
$ uvt-simplestreams-libvirt query
release=bionic arch=amd64 label=release (20190402)
release=xenial arch=amd64 label=release (20190406)
```

On our server, I don't update (sync) that often (every 2-4 months).

For more details:

*  uvtool syntax is [here](http://manpages.ubuntu.com/manpages/trusty/man1/uvt-kvm.1.html)
*  more readable tutorial is [here](https://help.ubuntu.com/lts/serverguide/cloud-images-and-uvtool.html)

## 2.2. apt-cacher-server installation ##

I recommend to follow to the letter this [tutorial](https://help.ubuntu.com/community/Apt-Cacher-Server).

The reason: we are creating/using/destroying a lot of VM instances and we are always installing the same packages.
This service allows to cache on the host and, doing so, **decreases the pressure on your internet bandwith usage**.
It also optimizes time at build stage.

```bash
$ sudo apt-get install apt-cacher apache2
$ sudo vi /etc/default/apt-cache
$ sudo service apache2 restart

# Server configuration
$ sudo vi /etc/apt-cacher/apt-cacher.conf 
 --> allowed_hosts = *
 --> fix the installer_files_regexp
$ sudo vi /etc/apt/apt.conf.d/01proxy
--> add `Acquire::http::Proxy "http://<IP address or hostname of the apt-cacher server>:3142";`
$ sudo service apt-cacher restart
```

This last file (/etc/apt/apt.conf.d/01proxy) is very important since it is tested in any CI script.

---

We can now switch to the next step: [how to deal with oai sources](./vm_based_simulator_sources.md)

You can also go back to the [CI dev main page](./ci_dev_home.md)

