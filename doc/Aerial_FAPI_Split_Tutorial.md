<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# OAI - Aerial FAPI Split Tutorial

**Table of Contents**

[[_TOC_]]

## Prerequisites

The hardware on which we have tried this tutorial:

| Hardware (CPU,RAM)                                                         |Operating System (kernel)                  | NIC (Vendor,Driver,Firmware)                     |
|----------------------------------------------------------------------------|----------------------------------|--------------------------------------------------|
| Gigabyte  Edge E251-U70 (Intel Xeon Gold 6240R, 2.4GHz, 24C48T, 96GB DDR4) |Ubuntu 22.04.3 LTS (5.15.0-72-lowlatency)| NVIDIA ConnectX®-6 Dx 22.38.1002                 |
| Dell PowerEdge R750 (Dual Intel Xeon Gold 6336Y CPU @ 2.4G, 24C/48T (185W), 512GB RDIMM, 3200MT/s) |Ubuntu 22.04.3 LTS (5.15.0-72-lowlatency)| NVIDIA Converged Accelerator A100X  (24.39.2048) |
| Supermicro Grace Hopper MGX ARS-111GL-NHR (Neoverse-V2, 3.4GHz, 72C/72T, 576GB LPDDR5) | Ubuntu 22.04.5 LTS (6.5.0-1019-nvidia-64k) |NVIDIA BlueField3 (32.41.1000)|

**Note**:
- These are not minimum hardware requirements. This is the configuration of our
  servers. The NIC card should support hardware PTP time stamping.
- Starting from tag
  [2025.w13](https://github.com/duranta-project/openairinterface5g/releases/tag/2025.w13)
  of OAI, we are only testing with the Grace Hopper server.

PTP enabled switches and grandmaster clock we have tested with:

| Vendor                   | Software Version |
|--------------------------|------------------|
| Fibrolan Falcon-RX/812/G | 8.0.25.4         |
| CISCO C93180YC-FX3       | 10.2(4)          |
| Qulsar Qg2 (Grandmaster) | 12.1.27          |


These are the radio units we've used for testing:

| Vendor                | Software Version            |
|-----------------------|-----------------------------|
| Foxconn RPQN-7801E RU | 2.6.9r254                   |
| Foxconn RPQN-7801E RU | 3.1.15_0p4                  |
| Foxconn RPQN-7801E RU | 3.2.0q.551.12.E.rc2.srs-AIO |
| WNC     R1220-078LE   | 1.9.0                       |

The UEs that have been tested and confirmed working with Aerial are the following:

| Vendor          | Model                         |
|-----------------|-------------------------------|
| Sierra Wireless | EM9191                        |
| Quectel         | RM500Q-GL                     |
| Quectel         | RM520N-GL                     |
| Apaltec         | Tributo 5G-Dongle             |
| OnePlus         | Nord (AC2003)                 |
| Apple iPhone    | 14 Pro (MQ0G3RX/A) (iOS 17.3) |
| Samsung         | S23 Ultra                     |


### Configure your server

To set up the L1 and install the components manually refer to this [instructions page](https://docs.nvidia.com/aerial/cuda-accelerated-ran/index.html).

**Note**:
- To configure the Gigabyte server please refer to these
  [instructions](https://github.com/duranta-project/openairinterface5g/blob/2025.w13/doc/Aerial_FAPI_Split_Tutorial.md)
- The last release to support the Gigabyte server is **Aerial CUDA-Accelerated
  RAN 24-1**.

#### CPU allocation

| Server brand     | Model         | Nº of CPU Cores | Isolated CPUs  |
|------------------|---------------|:---------------:|:--------------:|
| Grace Hopper MGX | ARS-111GL-NHR |      72         |      4-64      |

**Grace Hopper MGX ARS-111GL-NHR**

| Applicative Threads                  | Allocated CPUs                 |
|--------------------------------------|--------------------------------|
| `workers_ul`                         | 4, 5                           |
| `workers_dl`                         | 6, 7, 8                        |
| `timer_thread_config`                | 9                              |
| `message_thread_config`              | 9                              |
| `pcap_shm_caching_cpu_core`          | 10                             |
| `pcap_file_saving_cpu_core`          | 10                             |
| `dpdk_thread`                        | 10                             |
| `ul_pcap_capture_thread_cpu_affinity`| 10                             |
| `pcap_logger_thread_cpu_affinity`    | 10                             |
| `h2d_copy_thread_cpu_affinity`       | 11                             |
| `fh_stats_dump_cpu_core`             | -1                             |
| `pdump_client_thread`                | -1                             |
| `debug_worker`                       | -1                             |
| `prometheus_thread`                  | -1                             |
|                                      |                                |
| OAI `nr-softmodem`                   | 13,14,15,16                    |

**Note**:
- `-1` indicates that no explicit CPU pinning is applied and the thread
is scheduled by the Linux kernel.
- core 10 is a `low_priority_core` and it is shared by all low-priority threads.

#### PTP configuration

1. Install the `linuxptp` debian package. It will install both
   ptp4l and phc2sys.

```bash
#Ubuntu
sudo apt install linuxptp -y
```

Once installed you can use this configuration file for ptp4l
(`/etc/ptp4l.conf`). Here the clock domain is 24 so you can adjust it according
to your PTP GM clock domain

```
[global]
domainNumber            24
slaveOnly               1
time_stamping           hardware
tx_timestamp_timeout    30

[your_PTP_ENABLED_NIC]
network_transport       L2

```
The service of ptp4l (`/lib/systemd/system/ptp4l.service`) should be configured
as below:

```
[Unit] Description=Precision Time Protocol (PTP) service
Documentation=man:ptp4l
 
[Service]
Restart=always
RestartSec=5s
Type=simple
ExecStart=/usr/sbin/ptp4l -f /etc/ptp.conf
 
[Install]
WantedBy=multi-user.target

```

and service of phc2sys (`/lib/systemd/system/phc2sys.service`) should be
configured as below:

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
ExecStart=/usr/sbin/phc2sys -a -r -n 24

[Install]
WantedBy=multi-user.target

```
**Note**: As of release 25-3 there's no need to allocate a core for PTP.

## Prepare the L1

- Follow these
  [instructions](https://github.com/NVIDIA/aerial-cuda-accelerated-ran) to
  clone the `aerial-cuda-accelerated-ran` repository, pull cuBB image.
-  The CMake flags we use to build the SDK are:
  - `-DSCF_FAPI_10_04_SRS=ON` this flag must be used as of OAI tag `2025.w36`
    and this is due to the usage of the FAPI 10.04 version of the SRS PDU, and
    RX_Beamforming PDU.
  - `-DENABLE_CONFORMANCE_TM_PDSCH_PDCCH=OFF` this option should only be
    enabled when doing conformance testing with testmac.

```bash
./testBenches/phase4_test_scripts/build_aerial_sdk.sh --preset 10_02 -- -DSCF_FAPI_10_04_SRS=ON -DENABLE_CONFORMANCE_TM_PDSCH_PDCCH=OFF
```

## Build OAI gNB

If it's not already cloned, the first step is to clone OAI repository

```bash
git clone https://github.com/duranta-project/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g/
```

### Get nvIPC sources from the L1 container

The library used for communication between L1 and L2 components is called
nvIPC, and is developed by NVIDIA. In order to achieve this communication, we
need to obtain the nvIPC source files from the L1 container (cuBB) and place it
in the gNB project directory `~/openairinterface5g`. This allows us to build
and install this library when building the L2 docker image.

```bash
# Start interactive development container
./cuPHY-CP/container/run_aerial.sh

# Pack the nvIPC sources and copy them to the host ( the command creates a `tar.gz` file with the following name format: `nvipc_src.YYYY.MM.DD.tar.gz`)
aerial@c_aerial_<user>:/opt/nvidia/cuBB# cd cuPHY-CP/gt_common_libs
aerial@c_aerial_<user>:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs#./pack_nvipc.sh
nvipc_src.YYYY.MM.DD/ ... --------------------------------------------- Pack
nvipc source code finished:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs/nvipc_src.YYYY.MM.DD.tar.gz
```

Keep the interactive development container open and in another terminal use
`docker cp` to copy the library

```bash
docker cp c_aerial_oaicicd:/opt/nvidia/cuBB/cuPHY-CP/gt_common_libs/nvipc_src.YYYY.MM.DD.tar.gz ~/openairinterface/
```
*Important note:* For using docker cp, make sure to
copy the entire name of the created nvipc_src tar.gz file.

You can now exit the interactive development container.

With the nvIPC sources in the project directory, the L2 docker image can be built.


### Building OAI gNB docker image

In order to build the target image (`oai-gnb-aerial`), first you should build a
common shared image (`ran-base`). For more information about `docker build`
files please refer to this [tutorial](../docker/README.md)

```bash
~$ cd ~/openairinterface5g/
~/openairinterface5g$ docker build . -f docker/Dockerfile.base.ubuntu --tag ran-base:latest
~/openairinterface5g$ docker build . -f docker/Dockerfile.gNB.aerial.ubuntu --tag oai-gnb-aerial:latest
```


## Running the setup

### Adapt the OAI-gNB configuration file to your system/workspace

Edit the [OAI gNB configuration file](../ci-scripts/conf_files/gnb-vnf.sa.band78.273prb.aerial.conf)
and check the following parameters:

* `gNBs` section
  * The PLMN section shall match the one defined in the AMF
  * To calculate the `absoluteFrequencySSB` and `dl_absoluteFrequencyPointA`,
    please follow these [instructions](./gNB_frequency_setup.md)
  * `amf_ip_address` shall be the correct AMF IP address in your system
  * `GNB_IPV4_ADDRESS_FOR_NG_AMF` shall match your DU N2 interface IP address
  * `GNB_IPV4_ADDRESS_FOR_NGU` shall match your DU N3 interface IP address
  
* `Aerial` section (optional): physical antenna counts advertised to the L1

```
Aerial = {
  num_tx_ant = 4;  # CONFIG.request numTxAnt
  num_rx_ant = 4;  # CONFIG.request numRxAnt
};
```

These set the *physical* antenna counts in the FAPI CONFIG.request
(`numTxAnt`/`numRxAnt`), decoupled from the *logical* antenna ports
(`pdsch_AntennaPorts`/`pusch_AntennaPorts`, carried in `numTxPort`/`numRxPort`).
`numRxAnt` sizes cuBB's UL receive processing: e.g. on a 4x4 radio with
2-layer UL (`pusch_AntennaPorts = 2`), setting `num_rx_ant = 4` makes the
PUSCH MMSE/IRC equalizer combine all four RX antennas per layer (~3 dB array
gain) instead of only two. The fronthaul configuration must deliver the
corresponding number of UL eAxC streams. When the section is absent (or values
are 0), the previous behavior applies: physical counts are derived from the
logical antenna ports (or 64 when analog beamforming is preconfigured).

Note that the semantics change when cuBB runs with mMIMO enabled (cuBB-side
yaml setting): `numRxAnt` then only sizes SRS reception (physical antenna
elements), while PUSCH processing is dimensioned by the vendor-specific
`numRxPort`/`numTxPort` TLVs (0xA016/0xA017), which OAI fills with the logical
antenna port counts. Without those TLVs cuBB falls back to a compile-time
default of 16 ports.

The default amf_ip_address:ipv4 value is 192.168.70.132, when installing the
CN5G following [this tutorial](./NR_SA_Tutorial_OAI_CN5G.md)
Both `GNB_IPV4_ADDRESS_FOR_NG_AMF` and `GNB_IPV4_ADDRESS_FOR_NGU` need to be
set to the IP address of the NIC referenced previously.

**Note**: If the Core Network is running on the same server, 3 cores should be
allocated to it. 2 for the UPF and 1 core shared between the remaining services as shown
below.

```patch
diff --git a/docker-compose/docker-compose-basic-nrf.yaml b/docker-compose/docker-compose-basic-nrf.yaml
index 7fbefc9..d5ef964 100644
--- a/docker-compose/docker-compose-basic-nrf.yaml
+++ b/docker-compose/docker-compose-basic-nrf.yaml
@@ -28,6 +28,7 @@ services:
             - 8080/tcp
         volumes:
             - ./conf/basic_nrf_config.yaml:/openair-udr/etc/config.yaml
+        cpuset: "15"
         environment:
             - TZ=Europe/Paris
         depends_on:
```

### Aerial L1 entrypoint script

The `aerial_l1_entrypoint` script is used by the L1 container to start the L1
software and is mounted in the Docker Compose file. It begins by setting up
environment variables, restarting NVIDIA MPS, and finally running
`cuphycontroller_scf`.

The L1 software is executed with an argument that specifies which configuration
file to use. If not modified, the default argument is set to `P5G_WNC_GH`.

### Aerial L1 configuration

```bash
cd ~/aerial-cuda-accelerated-ran/cuPHY-CP/cuphycontroller/config/
```

Choose the configuration file that matches your setup and make the necessary
changes. We currently use `cuphycontroller_P5G_WNC_GH.yaml`.

Make sure that the following parameters are modified:

```
dst_mac_addr:
nic:
vlan:
```

### Docker compose

Apply the patch below to include the Aerial repository cloned earlier in this tutorial.
```patch
diff --git a/ci-scripts/yaml_files/sa_gnb_aerial/docker-compose.yaml b/ci-scripts/yaml_files/sa_gnb_aerial/docker-compose.yaml
index 985fe9a6a3..0774d826ac 100644
--- a/ci-scripts/yaml_files/sa_gnb_aerial/docker-compose.yaml
+++ b/ci-scripts/yaml_files/sa_gnb_aerial/docker-compose.yaml
@@ -15,10 +15,10 @@ services:
     stdin_open: true
     tty: true
     volumes:
+      - ~/aerial-cuda-accelerated-ran:/opt/nvidia/cuBB/
       - /lib/modules:/lib/modules
       - /dev/hugepages:/dev/hugepages
       - /usr/src:/usr/src
-      - ./cuphycontroller_P5G_WNC_GH.yaml:/opt/nvidia/cuBB/cuPHY-CP/cuphycontroller/config/cuphycontroller_P5G_WNC_GH.yaml
       - ./aerial_l1_entrypoint.sh:/opt/nvidia/cuBB/aerial_l1_entrypoint.sh
       - /var/log/aerial:/var/log/aerial
       - ../../../cmake_targets/share:/opt/cuBB/share
```

By applying this change, it would easier for the user to modify the L1 code or
configuration files directly from the `aerial-cuda-accelerated-ran` repository.

`ci-scripts/yaml_files/sa_gnb_aerial/docker-compose.yaml` is tailored for our
CI.
We copy the `aerial-cuda-accelerated-ran` repository inside the cuBB
docker image using `docker cp` and commit the changes using `docker commit`.
We mount the `cuphycontroller_P5G_WNC_GH.yaml` in the `nv-cubb` volumes so
that it would override some parameters depending on the setup being run on CI.

After building the gNB image, and preparing the configuration file, the setup
can be run with the following command:

```bash
cd ci-scripts/yaml_files/sa_gnb_aerial/
docker compose up -d
```
This will start both containers, beginning with `nv-cubb`, and
`oai-gnb-aerial` will start only after it is ready.

The logs can be followed using these commands:

```bash
docker logs -f oai-gnb-aerial
docker logs -f nv-cubb
```

#### Running with multiple L2s

One L1 instance can support multiple L2 instances. See also
the [aerial
documentation](https://docs.nvidia.com/aerial/cuda-accelerated-ran/latest/quickstart_guide/running_cubb-end-to-end.html#run-multiple-l2-instances-with-single-l1-instance)
for more details.

In OAI the shared memory prefix must be configured in the configuration file.

```bash
        tr_s_preference = "aerial";
	tr_s_shm_prefix = "nvipc";
```


#### Stopping the setup

Run the following command to stop and remove both containers, leaving the
system ready to be restarted later:

```bash
cd ci-scripts/yaml_files/sa_gnb_aerial/
docker compose down
```
