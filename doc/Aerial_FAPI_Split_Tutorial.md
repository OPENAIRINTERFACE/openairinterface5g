<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI 7.2 Fronthaul Interface 5G SA Tutorial</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

[[_TOC_]]

# Prerequisites

The hardware on which we have tried this tutorial:

| Hardware (CPU,RAM)                                                         |Operating System (kernel)                  | NIC (Vendor,Driver,Firmware)                     |
|----------------------------------------------------------------------------|----------------------------------|--------------------------------------------------|
| Gigabyte  Edge E251-U70 (Intel Xeon Gold 6240R, 2.4GHz, 24C48T, 96GB DDR4) |Ubuntu 22.04.3 LTS (5.15.0-72-lowlatency)| NVIDIA ConnectX®-6 Dx 22.38.1002                 |
| Dell PowerEdge R750 (Dual Intel Xeon Gold 6336Y CPU @ 2.4G, 24C/48T (185W), 512GB RDIMM, 3200MT/s) |Ubuntu 22.04.3 LTS (5.15.0-72-lowlatency)| NVIDIA Converged Accelerator A100X  (24.39.2048) |

**NOTE**: These are not minimum hardware requirements. This is the configuration of our servers. The NIC card should support hardware PTP time stamping.

PTP enabled switches and grandmaster clock we have tested with:

| Vendor                   | Software Version |
|--------------------------|------------------|
| Fibrolan Falcon-RX/812/G | 8.0.25.4         |
| CISCO C93180YC-FX3       | 10.2(4)          |
| Qulsar Qg2 (Grandmaster) | 12.1.27          |


These are the radio units we've used for testing:

| Vendor      | Software Version |
|-------------|------------------|
| Foxconn RPQN-7801E RU | 2.6.9r254        |
| Foxconn RPQN-7801E RU | 3.1.15_0p4       |


The UEs that have been tested and confirmed working with Aerial are the following:

| Vendor          | Model                         |
|-----------------|-------------------------------|
| Sierra Wireless | EM9191                        |
| Quectel         | RM500Q-GL                     |
| Apaltec         | Tributo 5G-Dongle             |
| OnePlus         | Nord (AC2003)                 |
| Apple iPhone    | 14 Pro (MQ0G3RX/A) (iOS 17.3) |


## Configure your server

The first step is to obtain the NVIDIA Aerial SDK, you'll need to request access [here](https://catalog.ngc.nvidia.com/orgs/nvidia/containers/aerial-sdk).

After obtaining access to the SDK, you can refer to [this instructions page](https://docs.nvidia.com/aerial/aerial-research-cloud/current/index.html) to setup the L1 components using NVIDIAs' SDK manager, or to [this instructions page](https://developer.nvidia.com/docs/gputelecom/aerial-sdk/aerial-sdk-archive/aerial-sdk-23-2/index.html) in order to setup and install the components manually.

The currently used Aerial Version is 23-2, which is also the one currently used by the SDK Manager.


### CPU allocation

Currently, the CPU isolation is setup the following:

| Server brand | Model     | Nº of CPU Cores | Isolated CPUs  |
|--------------|-----------|:---------------:|:--------------:|
| Gigabyte     | Edge E251-U70 |      24     |      2-10      |


**Gigabyte  Edge E251-U70**

| Applicative Threads    | Allocated CPUs |
|------------------------|----------------|
| Aerial L1              | 2,3,4,5,6,7,8  |
| PTP & PHC2SYS Services | 9              |
| OAI `nr-softmodem`     | 13-20          |


## PTP configuration

1. You need to install the `linuxptp` debian package. It will install both ptp4l and phc2sys.

```bash
#Ubuntu
sudo apt install linuxptp -y
```

Once installed you can use this configuration file for ptp4l (`/etc/ptp4l.conf`). Here the clock domain is 24 so you can adjust it according to your PTP GM clock domain

```
[global]
domainNumber            24
slaveOnly               1
time_stamping           hardware
tx_timestamp_timeout    1
logging_level           6
summary_interval        0
#priority1               127 

[your_PTP_ENABLED_NIC]
network_transport       L2
hybrid_e2e              0

```

The service of ptp4l (`/lib/systemd/system/ptp4l.service`) should be configured as below:
```
[Unit]
Description=Precision Time Protocol (PTP) service
Documentation=man:ptp4l
 
[Service]
Restart=always
RestartSec=5s
Type=simple
ExecStart=taskset -c 9 /usr/sbin/ptp4l -f /etc/ptp.conf
 
[Install]
WantedBy=multi-user.target

```

and service of phc2sys (`/lib/systemd/system/phc2sys.service`) should be configured as below:
```
[Unit]
Description=Synchronize system clock or PTP hardware clock (PHC)
Documentation=man:phc2sys
After=ntpdate.service
Requires=ptp4l.service
After=ptp4l.service

[Service]
Restart=always
RestartSec=5s
Type=simple
ExecStart=/usr/sbin/phc2sys -a -r -r -n 24

[Install]
WantedBy=multi-user.target

```

# Build OAI gNB

If installing with the Aerial SDK, you should already have the repository cloned in `~/openairinterface5g`, if 
Clone OAI code base in a suitable repository, here we are cloning in `~/openairinterface5g` directory,

```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g/
git checkout Aerial_Integration
```

## Get nvIPC sources from the L1 container 
The library used for communication between L1 and L2 components is called nvIPC, and was developed by NVIDIA, as such, it is not open-source and can't be freely distributed.
In order to be able to use this library to achieve communication, we need to obtain the nvIPC source files from the L1 container (cuBB) and place it in out gNB project directory `~/openairinterface5g`.
This allows us to build and install this library when building the L2 docker container.

Check whether your L1 container is running:
```bash
~$ docker container ls -a
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS                      PORTS               NAMES
a9681a0c4a10        14dca2002237        "/opt/nvidia/nvidia_…"   3 months ago        Exited (137) 10 days ago                        cuBB

```
If it is not running, you may start it and logging into the container by running the following:
```bash
~$ docker start cuBB
cuBB
~$ docker exec -it cuBB bash
root@c_aerial_aerial:/opt/nvidia/cuBB# 
```

After logging into the container, we need to pack the nvIPC sources and copy them to the host ( the command creates a tar.gz file with the following name format: nvipc_src.yyyy.mm.dd.tar.gz)
```bash
~$ docker exec -it cuBB bash
root@c_aerial_aerial:/opt/nvidia/cuBB# cd cuPHY-CP/gt_common_libs
root@c_aerial_aerial:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs#./pack_nvipc.sh 
nvipc_src.****.**.**/
...
---------------------------------------------
Pack nvipc source code finished:
/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs/nvipc_src.****.**.**.tar.gz
root@c_aerial_aerial:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs# cp nvipc_src.****.**.**.tar.gz /opt/cuBB/share/
root@c_aerial_aerial:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs# exit
```

The file should now be present in the `~/openairinterface5g/ci-scripts/yaml_files/sa_gnb_aerial/` directory, from where it is moved into `~/openairinterface5g`
```bash
~$ mv ~/openairinterface5g/ci-scripts/yaml_files/sa_gnb_aerial/nvipc_src.****.**.**.tar.gz ~/openairinterface5g/
```
With the nvIPC sources in the project directory, the docker image can be built.

## Building OAI gNB docker image

In order to build the final image, there is an intermediary image to be built (ran-base)
```bash
~$ cd ~/openairinterface5g/
~/openairinterface5g$ docker build . -f docker/Dockerfile.base.ubuntu20 --tag ran-base:latest
~/openairinterface5g$ docker build . -f docker/Dockerfile.gNB.aerial.ubuntu20 --tag oai-gnb-aerial:latest
```


## Running the setup
In order to use Docker compose to automatically start and stop the setup, we need to first create a ready-made image of the L1, after compiling the L1 software and making the necessary adjustments to its configuration files.
The process of preparing the L1 is covered on NVIDIAs' documentation, and falls outside the scope of this document.

### Prepare the L1 image
After preparing the L1 software, the container needs to be committed in order for an image in which the L1 is ready to be executed, and that can be referenced by a docker-compose.yaml file later:
```bash
~$ docker commit nv-cubb cubb-build:23-2
~$ docker image ls
..
cubb-build                                    23-2                                           824156e0334c   2 weeks ago    40.1GB
..
```

## Adapt the OAI-gNB configuration file to your system/workspace

Edit the sample OAI gNB configuration file and check following parameters:

* `gNBs` section
  * The PLMN section shall match the one defined in the AMF
  * `amf_ip_address` shall be the correct AMF IP address in your system
  * `GNB_INTERFACE_NAME_FOR_NG_AMF` and `GNB_IPV4_ADDRESS_FOR_NG_AMF` shall match your DU N2 interface name and IP address
  * `GNB_INTERFACE_NAME_FOR_NGU` and `GNB_IPV4_ADDRESS_FOR_NGU` shall match your DU N3 interface name and IP address
  
The default amf_ip_address:ipv4 value is 192.168.70.132, when installing the CN5G following [this tutorial](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/doc/NR_SA_Tutorial_OAI_CN5G.md)
Both 'GNB_INTERFACE_NAME_FOR_NG_AMF' and 'GNB_INTERFACE_NAME_FOR_NGU' need to be set to the network interface name used by the gNB host to connect to the CN5G.
Both 'GNB_IPV4_ADDRESS_FOR_NG_AMF' and 'GNB_IPV4_ADDRESS_FOR_NGU' need to be set to the IP address of the NIC referenced previously.


### Running docker compose
#### Aerial L1 entrypoint script
Before running docker-compose, we can check which L1 configuration file is to be used by cuphycontroller_scf, this is set in the script [`aerial_l1_entrypoint.sh`](../ci-scripts/yaml_files/sa_gnb_aerial/aerial_l1_entrypoint.sh), which is used by the L1 container in order to start the L1 software, this begins by installing a module 'gdrcopy', setting up some environment variables, restarting NVIDIA MPS, and finally running cuphycontroller with an argument that represent which L1 configuration file is to be used, this argument may be changed by providing an argument in docker-compose.yaml.
When no argument is provided (this is the default behaviour), it uses "P5G_SCF_FXN" as a cuphycontroller_scf  argument.

After building the gNB image, and preparing the configuration file, the setup can be run with the following command:

```bash
cd ci-scripts/yaml_files/sa_gnb_aerial/
docker compose up -d
 
```
This will start both containers, beginning with 'nv-cubb' and only after it being ready it starts 'oai-gnb-aerial'.

The gNB logs can be followed with:

```bash
docker logs -f oai-gnb-aerial
```

### Stopping the setup

Running the following command, will stop both containers, leaving the system ready to be run later:
```bash
cd ci-scripts/yaml_files/sa_gnb_aerial/
docker compose down
 
```
