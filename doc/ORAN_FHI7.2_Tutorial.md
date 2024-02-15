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

|Hardware (CPU,RAM)                          |Operating System (kernel)                  |NIC (Vendor,Driver,Firmware)                     |
|--------------------------------------------|----------------------------------|-------------------------------------------------|
|Intel(R) Xeon(R) Gold 6354 36-Core, 128GB   |RHEL 9.2 (5.14.0-284.18.1.rt14.303.el9_2.x86_64)|Intel X710, i40e, 9.20 0x8000d95e 22.0.9|
|Intel(R) Xeon(R) Gold 6354 36-Core, 128GB   |Ubuntu 22.04.3 LTS (5.15.0-1033-realtime)|Intel X710, i40e, 9.00 0x8000cfeb 21.5.9|
|AMD EPYC 9374F 32-Core Processor, 128GB      |Ubuntu 22.04.2 LTS (5.15.0-1038-realtime)       |Intel E810 ,ice, 4.00 0x8001184e 1.3236.0   |

**NOTE**: These are not minimum hardware requirements. This is the configuration of our servers. The NIC card should support hardware PTP time stamping.

NICs we have tested so far:

|Vendor         |Firmware Version        |
|---------------|------------------------|
|Intel X710     |9.20 0x8000d95e 22.0.9  |
|Intel E810-XXV |4.00 0x8001184e 1.3236.0|
|E810-C         |4.20 0x8001784e 22.0.9  |
|Intel XXV710   |6.02 0x80003888         |

PTP enabled switches and grandmaster clock we have in are lab:

|Vendor                  |Software Version|
|------------------------|----------------|
|CISCO C93180YC-FX3      |10.2(4)         |
|Fibrolan Falcon-RX/812/G|8.0.25.4        |
|Qulsar Qg2 (Grandmaster)|12.1.27         |

**S-Plane synchronization is mandatory.** S-plane support is done via `ptp4l`
and `phc2sys`.

| Software  | Software Version |
|-----------|------------------|
| `ptp4l`   | 3.1.1            |
| `phc2sys` | 3.1.1            |

We have only verified LLS-C3 configuration in our lab, i.e.  using an external
grandmaster, a switch as a boundary clock, and the gNB/DU and RU.  We haven't
tested any RU without S-plane. Radio units we are testing/integrating:

|Vendor     |Software Version |
|-----------|-----------------|
|VVDN LPRU  |03-v3.0.4        |
|LiteON RU  |01.00.08/02.00.03|
|Benetel 650|v0.8.1           |

Tested libxran releases:

| Vendor                                  |
|-----------------------------------------|
| `oran_e_maintenance_release_v1.0`       |

**Note**: the libxran driver of OAI identifies the above version as "5.1.0" (E is fifth letter, then 1.0).

## Configure your server

1. Disable Hyperthreading (HT) in your BIOS. In all our servers HT is always disabled.
2. We recommend you to start with a fresh installation of OS (either RHEL or Ubuntu). You have to install realtime kernel on your OS (Operating System). Based on your OS you can search how to install realtime kernel.
3. Install realtime kernel for your OS
4. Change the boot commands based on the below section. They can be performed either via `tuned` or via manually building the kernel

### CPU allocation

**This section is important to read, regardless of the operating system you are using.**

Your server could be:

* One NUMA node (See [one NUMA node example](#111-one-numa-node)): all the processors are sharing a single memory system.
* Two NUMA nodes (see [two NUMA nodes example](#112-two-numa-node)): processors are grouped in 2 memory systems.
  - Usually the even (ie `0,2,4,...`) CPUs are on the 1st socket
  - And the odd (ie (`1,3,5,...`) CPUs are on the 2nd socket

DPDK, OAI and kernel threads require to be properly allocated to extract maximum real-time performance for your use case.

1. **NOTE**: Currently the default OAI 7.2 configuration file requires isolated **CPUs 0,2,4** for DPDK/libXRAN, **CPU 6** for `ru_thread`, **CPU 8** for `L1_rx_thread` and **CPU 10** for `L1_tx_thread`. It is preferrable to have all these threads on the same socket.
2. Allocating CPUs to the OAI nr-softmodem is done using the `--thread-pool` option. Allocating 4 CPUs is the minimal configuration but we recommend to allocate at least **8** CPUs. And they can be on a different socket as the DPDK threads.
3. And to avoid kernel preempting these allocated CPUs, it is better to force the kernel to use un-allocated CPUs.

Let summarize for example on a `32-CPU` single NUMA node system, regardless of the number of sockets:

|Applicative Threads|Allocated CPUs    |
|-------------------|------------------|
|XRAN DPDK usage    |0,2,4             |
|OAI `ru_thread`    |6                 |
|OAI `L1_rx_thread` |8                 |
|OAI `L1_tx_thread` |10                |
|OAI `nr-softmodem` |1,3,5,7,9,11,13,15|
|kernel             |16-31             |

In below example we have shown the output of `/proc/cmdline` for two different servers, each of them have different number of NUMA nodes. **Be careful in isolating the CPUs in your environment.** Apart from CPU allocation there are additional parameters which are important to be present in your boot command.

Modifying the `linux` command line usually requires to edit string `GRUB_CMDLINE_LINUX` in `/etc/default/grub`, run a `grub` command and reboot the server.

* Set parameters `isolcpus`, `nohz_full` and `rcu_nocbs` with the list of CPUs to isolate for XRAN.
* Set parameter `kthread_cpus` with the list of CPUs to isolate for kernel.

Set the `tuned` profile to `realtime`. If the `tuned-adm` command is not installed then you have to install it. When choosing this profile you have to mention the isolated cpus in `/etc/tuned/realtime-variables.conf`. By default this profile adds `skew_tick=1 isolcpus=managed_irq,domain,<cpu-you-choose> intel_pstate=disable nosoftlockup tsc=nowatchdog` in the boot command. **Make sure you don't add them while changing `/etc/default/grub`**.

```bash
tuned-adm profile realtime
```

**Checkout anyway the examples below.**

### One NUMA Node

Below is the output of `/proc/cmdline` of a single NUMA node server,

```bash
NUMA:
  NUMA node(s):          1
  NUMA node0 CPU(s):     0-31
```

```bash
isolcpus=0-15 nohz_full=0-15 rcu_nocbs=0-15 kthread_cpus=16-31 rcu_nocb_poll nosoftlockup default_hugepagesz=1GB hugepagesz=1G hugepages=20 amd_iommu=on iommu=pt mitigations=off skew_tick=1 selinux=0 enforcing=0 tsc=nowatchdog nmi_watchdog=0 softlockup_panic=0 audit=0 vt.handoff=7
```

Example taken for AMD EPYC 9374F 32-Core Processor

### Two NUMA Nodes

Below is the output of `/proc/cmdline` of a two NUMA node server,

```
NUMA:
  NUMA node(s):          2
  NUMA node0 CPU(s):     0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34
  NUMA node1 CPU(s):     1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35
```

```bash
mitigations=off usbcore.autosuspend=-1 intel_iommu=on intel_iommu=pt selinux=0 enforcing=0 nmi_watchdog=0 softlockup_panic=0 audit=0 skew_tick=1 isolcpus=managed_irq,domain,0,2,4,6,8,10,12,14,16 nohz_full=0,2,4,6,8,10,12,14,16 rcu_nocbs=0,2,4,6,8,10,12,14,16 rcu_nocb_poll intel_pstate=disable nosoftlockup tsc=nowatchdog cgroup_disable=memory mce=off hugepagesz=1G hugepages=40 hugepagesz=2M hugepages=0 default_hugepagesz=1G isolcpus=managed_irq,domain,0,2,4,6,8,10,12,14 kthread_cpus=18-35 intel_pstate=disable nosoftlockup tsc=reliable
```

Example taken for Intel(R) Xeon(R) Gold 6354 CPU @ 3.00GHz

### Common

Configure your servers to maximum performance mode either via OS or in BIOS. If you want to disable CPU sleep state via OS then use the below command:

```bash
# to disable
sudo cpupower idle-set -D 0
#to enable
sudo cpupower idle-set -E
```

The above information we have gathered either from O-RAN documents or via our own experiments. In case you would like to read the O-RAN documents then here are the links:

1. [O-RAN-SC O-DU Setup Configuration](https://docs.o-ran-sc.org/projects/o-ran-sc-o-du-phy/en/latest/Setup-Configuration_fh.html)
2. [O-RAN Cloud Platform Reference Designs 2.0,O-RAN.WG6.CLOUD-REF-v02.00,February 2021](https://orandownloadsweb.azurewebsites.net/specifications)


## PTP configuration

**Note**: You may run OAI with O-RAN 7.2 Fronthaul without a RU attached (e.g. for benchmarking).
In such case, you can skip PTP configuration and go to DPDK section.

1. You can install `linuxptp` rpm or debian package. It will install ptp4l and phc2sys.

```bash
#RHEL
sudo dnf install linuxptp -y
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
Probably you need to increase `tx_timestamp_timeout` to 50 or 100 for Intel E-810. You will see that in the logs of ptp.

Create the configuration file for phc2sys (`/etc/sysconfig/phc2sys`)

```
OPTIONS="-a -r -r -n 24"
```

The service of ptp4l (`/usr/lib/systemd/system/ptp4l.service`) should be configured as below:

```
[Unit]
Description=Precision Time Protocol (PTP) service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
EnvironmentFile=-/etc/sysconfig/ptp4l
ExecStart=/usr/sbin/ptp4l $OPTIONS

[Install]
WantedBy=multi-user.target
```

and service of phc2sys (`/usr/lib/systemd/system/phc2sys.service`) should be configured as below:

```
[Unit]
Description=Synchronize system clock or PTP hardware clock (PHC)
After=ntpdate.service ptp4l.service

[Service]
Type=simple
EnvironmentFile=-/etc/sysconfig/phc2sys
ExecStart=/usr/sbin/phc2sys $OPTIONS

[Install]
WantedBy=multi-user.target
```

### Debugging PTP issues

You can see these steps in case your ptp logs have erorrs or `rms` reported in `ptp4l` logs is more than 100ms.
Beware that PTP issues may show up only when running OAI and XRAN. If you are using the `ptp4l` service, have a look back in time in the journal: `journalctl -u ptp4l.service -S <hours>:<minutes>:<seconds>`

1. Make sure that you have `skew_tick=1` in `/proc/cmdline`
2. For Intel E-810 cards set `tx_timestamp_timeout` to 50 or 100 if there are errors in ptp4l logs
3. Other time sources than PTP, such as NTP or chrony timesources, should be disabled. Make sure they are enabled as further below.
4. Make sure you set `kthread_cpus=<cpu_list>` in `/proc/cmdline`.
5. If `rms` or `delay` in `ptp4l` or `offset` in `phc2sys` logs remain high then you can try pinning the `ptp4l` and `phc2sys` processes to an isolated CPU.

```bash
#to check there is NTP enabled or not
timedatectl | grep NTP
#to disable
timedatectl set-ntp false
```

## DPDK (Data Plane Development Kit)

Download DPDK version 20.11.7.

```bash
# on debian
sudo apt install wget xz-utils
# on Fedora/RHEL
sudo dnf install wget xz
cd
wget http://fast.dpdk.org/rel/dpdk-20.11.7.tar.xz
```

### DPDK Compilation and Installation

```bash
# Installing meson : it should pull ninja-build and compiler packages
# on debian
sudo apt install meson
# on Fedora/RHEL
sudo dnf install meson
tar xvf dpdk-20.11.7.tar.xz && cd dpdk-stable-20.11.7

meson build
ninja -C build
sudo ninja install -C build
```

### Verify the installation is complete

Check if the LD cache contains the DPDK Shared Objects after update:

```bash
sudo ldconfig -v | grep rte_
	librte_fib.so.0.200.2 -> librte_fib.so.0.200.2
	librte_telemetry.so.0.200.2 -> librte_telemetry.so.0.200.2
	librte_compressdev.so.0.200.2 -> librte_compressdev.so.0.200.2
	librte_gro.so.20.0 -> librte_gro.so.20.0.2
	librte_mempool_dpaa.so.20.0 -> librte_mempool_dpaa.so.20.0.2
	librte_distributor.so.20.0 -> librte_distributor.so.20.0.2
	librte_rawdev_dpaa2_cmdif.so.20.0 -> librte_rawdev_dpaa2_cmdif.so.20.0.2
	librte_mempool.so.20.0 -> librte_mempool.so.20.0.2
	librte_pmd_octeontx2_crypto.so.20.0 -> librte_pmd_octeontx2_crypto.so.20.0.2
	librte_common_cpt.so.20.0 -> librte_common_cpt.so.20.0.2
....
```

You may not have the `/usr/local/lib`, `/usr/local/lib64`, or custom DPDK
installation paths in the `LD_LIBRARY_PATH`. In this case, add it as below; if
you installed into a
custom

```bash
sudo echo "/usr/local/lib" > /etc/ld.so.conf.d/local-lib.conf
sudo echo "/usr/local/lib64" >> /etc/ld.so.conf.d/local-lib.conf
sudo ldconfig
sudo ldconfig -v | grep rte_
```

Check if the PKG-CONFIG tool discovers the libraries:

```bash
pkg-config --libs libdpdk --static
```

<details>
<summary>Possible output</summary>

```console
-lrte_node -lrte_graph -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -Wl,--whole-archive -lrte_common_cpt -lrte_common_dpaax -lrte_common_iavf -lrte_common_octeontx -lrte_common_octeontx2 -lrte_bus_dpaa -lrte_bus_fslmc -lrte_bus_ifpga -lrte_bus_pci -lrte_bus_vdev -lrte_bus_vmbus -lrte_mempool_bucket -lrte_mempool_dpaa -lrte_mempool_dpaa2 -lrte_mempool_octeontx -lrte_mempool_octeontx2 -lrte_mempool_ring -lrte_mempool_stack -lrte_pmd_af_packet -lrte_pmd_ark -lrte_pmd_atlantic -lrte_pmd_avp -lrte_pmd_axgbe -lrte_pmd_bond -lrte_pmd_bnxt -lrte_pmd_cxgbe -lrte_pmd_dpaa -lrte_pmd_dpaa2 -lrte_pmd_e1000 -lrte_pmd_ena -lrte_pmd_enetc -lrte_pmd_enic -lrte_pmd_failsafe -lrte_pmd_fm10k -lrte_pmd_i40e -lrte_pmd_hinic -lrte_pmd_hns3 -lrte_pmd_iavf -lrte_pmd_ice -lrte_pmd_igc -lrte_pmd_ixgbe -lrte_pmd_kni -lrte_pmd_liquidio -lrte_pmd_memif -lrte_pmd_netvsc -lrte_pmd_nfp -lrte_pmd_null -lrte_pmd_octeontx -lrte_pmd_octeontx2 -lrte_pmd_pfe -lrte_pmd_qede -lrte_pmd_ring -lrte_pmd_sfc -lrte_pmd_softnic -lrte_pmd_tap -lrte_pmd_thunderx -lrte_pmd_vdev_netvsc -lrte_pmd_vhost -lrte_pmd_virtio -lrte_pmd_vmxnet3 -lrte_rawdev_dpaa2_cmdif -lrte_rawdev_dpaa2_qdma -lrte_rawdev_ioat -lrte_rawdev_ntb -lrte_rawdev_octeontx2_dma -lrte_rawdev_octeontx2_ep -lrte_rawdev_skeleton -lrte_pmd_caam_jr -lrte_pmd_dpaa_sec -lrte_pmd_dpaa2_sec -lrte_pmd_nitrox -lrte_pmd_null_crypto -lrte_pmd_octeontx_crypto -lrte_pmd_octeontx2_crypto -lrte_pmd_crypto_scheduler -lrte_pmd_virtio_crypto -lrte_pmd_octeontx_compress -lrte_pmd_qat -lrte_pmd_ifc -lrte_pmd_dpaa_event -lrte_pmd_dpaa2_event -lrte_pmd_octeontx2_event -lrte_pmd_opdl_event -lrte_pmd_skeleton_event -lrte_pmd_sw_event -lrte_pmd_dsw_event -lrte_pmd_octeontx_event -lrte_pmd_bbdev_null -lrte_pmd_bbdev_turbo_sw -lrte_pmd_bbdev_fpga_lte_fec -lrte_pmd_bbdev_fpga_5gnr_fec -Wl,--no-whole-archive -lrte_node -lrte_graph -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -Wl,-Bdynamic -pthread -lm -ldl
```
</details>

If DPDK was installed into `/usr/local/lib`, `/usr/local/lib64`, or another
custom path, you have to point to the right directory with `PKG_CONFIG_PATH`,
for instance:

```bash
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig/
pkg-config --libs libdpdk --static
```

### If you want to de-install this version of DPDK

Go back to the version folder you used to build and install

```
cd ~/dpdk-stable-20.11.7
sudo ninja deinstall -C build
```

# Build OAI-FHI gNB

Clone OAI code base in a suitable repository, here we are cloning in `~/openairinterface5g` directory,

```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g/
```

## Build ORAN Fronthaul Interface Library

Download ORAN FHI DU library and checkout the correct version.

```bash
git clone https://gerrit.o-ran-sc.org/r/o-du/phy.git ~/phy
cd ~/phy
git checkout oran_e_maintenance_release_v1.0
```

Apply the patch (available in `oai_folder/cmake_targets/tools/oran_fhi_integration_patches/E`):

```bash
git apply ~/openairinterface5g/cmake_targets/tools/oran_fhi_integration_patches/E/oaioran_E.patch
```

Compile the fronthaul interface library by calling `make` and the option
`XRAN_LIB_SO=1` to have it build a shared object. Note that we provide two
environment variables `RTE_SDK` for the path to the source tree of DPDK, and
`XRAN_DIR` to set the path to the fronthaul library.

```bash
cd ~/phy/fhi_lib/lib
make clean
RTE_SDK=~/dpdk-stable-20.11.7/ XRAN_DIR=~/phy/fhi_lib make XRAN_LIB_SO=1
...
[AR] build/libxran.so
./build/libxran.so
```

The shared library object `~/phy/fhi_lib/lib/build/libxran.so` must be present
before proceeding.

## Build OAI gNB

You can now proceed building OAI. You build it the same way as for other
radios, providing option `-t oran_fhlib_5g`. Additionally, you need to provide
it the location of the FH library: `--cmake-opt -Dxran_LOCATION=PATH`. Note
that since we cannot use `~` here, we resort to `$HOME`, which is equivalent.
Finally, if you needed to define `PKG_CONFIG_PATH` previously, you need to do
so now, too.

```bash
# You should have already cloned above
cd ~/openairinterface5g/cmake_targets
# on debian
sudo apt install -y libnuma-dev
# on RHEL
sudo dnf install -y numactl-devel
export PKG_CONFIG_PATH=/opt/dpdk/lib64/pkgconfig/
./build_oai -I  # if you never installed OAI, use this command once before the next line
./build_oai --gNB --ninja -t oran_fhlib_5g --cmake-opt -Dxran_LOCATION=$HOME/phy/fhi_lib/lib
```

You can optionally check that everything has been linked properly with
```bash
ldd ran_build/build/liboran_fhlib_5g.so
```

<details>
<summary>Possible output</summary>

```console
#check if all the libraries are properly linked to liboai_transpro.so
ldd ran_build/build/liboran_fhlib_5g.so
    linux-vdso.so.1 (0x00007fffb459e000)
    librte_node.so.21 => /usr/local/lib64/librte_node.so.21 (0x00007fd358690000)
    librte_graph.so.21 => /usr/local/lib64/librte_graph.so.21 (0x00007fd358685000)
    librte_bpf.so.21 => /usr/local/lib64/librte_bpf.so.21 (0x00007fd358672000)
    librte_flow_classify.so.21 => /usr/local/lib64/librte_flow_classify.so.21 (0x00007fd35866c000)
    librte_pipeline.so.21 => /usr/local/lib64/librte_pipeline.so.21 (0x00007fd35862f000)
    librte_table.so.21 => /usr/local/lib64/librte_table.so.21 (0x00007fd358612000)
    librte_port.so.21 => /usr/local/lib64/librte_port.so.21 (0x00007fd3585f8000)
    librte_fib.so.21 => /usr/local/lib64/librte_fib.so.21 (0x00007fd3585e9000)
...
    libm.so.6 => /lib64/libm.so.6 (0x00007fd357eb1000)
    libnuma.so.1 => /lib64/libnuma.so.1 (0x00007fd357ea1000)
    libc.so.6 => /lib64/libc.so.6 (0x00007fd357c98000)
    /lib64/ld-linux-x86-64.so.2 (0x00007fd3587c7000)
    libelf.so.1 => /lib64/libelf.so.1 (0x00007fd357c7d000)
    libz.so.1 => /lib64/libz.so.1 (0x00007fd357c61000)
```
</details>

Note that you might also call cmake directly instead of using `build_oai`:
```
cd ~/openairinterface5g
mkdir build && cd build
cmake .. -GNinja -DOAI_FHI72=ON -Dxran_LOCATION=$HOME/phy/fhi_lib/lib
ninja nr-softmodem oran_fhlib_5g params_libconfig
```

# Configuration

**Note**: You may run OAI with O-RAN 7.2 Fronthaul without a RU attached (e.g. for benchmarking).
In such case, skip RU configuration and go through Network Interfaces, DPDK VFs and OAI configuration by using arbitrary values for RU MAC addresses and VLAN tags.

## Configure the RU

Contact the RU vendor to get the configuration manual, and configure the RU
appropriately. You can orient on the OAI configuration files mentioned further
below.

We are evaluating if we can share RU configuration steps.

## Configure Network Interfaces and DPDK VFs

The 7.2 fronthaul uses the xran library, which requires DPDK. In this step, we
need to configure network interfaces to send data to the RU, and configure DPDK
to bind to the corresponding PCI interfaces. More specifically, in the
following we use [SR-IOV](https://en.wikipedia.org/wiki/Single-root_input/output_virtualization)
to create multiple virtual functions (VFs) through which Control plane (C
plane) and User plane (U plane) traffic will flow. The following commands are
not persistant, and have to be repeated after reboot.

In the following, we will use these short hands:

- `physical-interface`: Physical network interface through which you can access the RU
- `vlan`: VLAN tags as defined in the RU configuration
- `mtu`: the MTU as specified by the RU vendor, and supported by the NIC
- `du-c-plane-mac-addr`: DU C plane MAC address
- `lspci-address-c-plane-vf`: PCI bus address of the VF for C plane
- `du-u-plane-mac-addr`: DU U plane MAC address
- `lspci-address-u-plane-vf`: PCI bus address of the VF for U plane

For both the MAC addresses, you might use the MAC addresses which are
pre-configured in the RUs (typically `00:11:22:33:44:66`, but that is not
always the case). Note that if your system has Intel E-810 NIC cards/ICE
driver, you have to choose different MAC addresses (valid for above-mentioned
kernels). If the RU vendor requires untagged traffic, remove the VLAN tagging
in the below command and configure VLAN on the switch as "access VLAN". In case
the MTU is different than 1500, you have to update the MTU on the switch
interface as well.

First, set maximum ring buffers:
```bash
sudo ethtool -g <physical-interface>
sudo ethtool -G <physical-interface> rx 4096     # assumes 4096 is max
sudo ethtool -G <physical-interface> tx 4096     # assumes 4096 is max
```

Set the maximum MTU in the physical interface:
```bash
sudo ifconfig <physical-interface> mtu <mtu>
```

(Re-)create two VFs, load the Linux "Base Driver for Intel Ethernet Adaptive
Virtual Function" (in case you use Intel ethernet card), and set up the VFs.

```bash
sudo modprobe iavf
sudo sh -c echo "0" > /sys/class/net/<physical-interface>/device/sriov_numvfs
sudo sh -c echo "2" > /sys/class/net/<physical-interface>/device/sriov_numvfs
sudo ip link set <physical-interface> vf 0 mac <du-c-plane-mac-addr> vlan <vlan> mtu <mtu> spoofchk off
sudo ip link set <physical-interface> vf 1 mac <du-u-plane-mac-addr> vlan <vlan> mtu <mtu> spoofchk off
```

After running the above commands, the kernel created virtual functions that
have been assigned a PCI address under the same device and vendor ID. For
instance, use `sudo lshw -c network -businfo` to get a list of PCI addresses
and interface names, locate the PCI address of `<physical-interface>`, then use
`lspci | grep Virtual` to get all virtual interfaces and use the ones with the
same Device/Vendor ID parts (first two numbers).

<details>
<summary>Example</summary>

The machine in this example has an Intel X710 card. The interface
<physical-interface> in question is `eno12409`. Running `lshw` gives:

```bash
$ sudo lshw -c network -businfo
Bus info          Device     Class          Description
=======================================================
[...]
pci@0000:31:00.1  eno12409   network        Ethernet Controller X710 for 10GbE SFP+
[...]
```

We see the PCI address `31:00.1`. Listing the virtual interfaces through
`lspci`, we get

```bash
$ lspci | grep Virtual
31:06.0 Ethernet controller: Intel Corporation Ethernet Virtual Function 700 Series (rev 02)
31:06.1 Ethernet controller: Intel Corporation Ethernet Virtual Function 700 Series (rev 02)
98:11.0 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
98:11.1 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
98:11.2 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
98:11.3 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
98:11.4 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
98:11.5 Ethernet controller: Intel Corporation Ethernet Adaptive Virtual Function (rev 02)
```

The hardware card `31:00.1` has two associated virtual functions `31:06.0` and
`31:06.1`.
</details>

Now, unbind any pre-existing DPDK devices, load the "Virtual Function I/O"
driver `vfio_pci`, and bind DPDK to these devices:

```
sudo /usr/local/bin/dpdk-devbind.py --unbind <lspci-address-c-plane-vf>
sudo /usr/local/bin/dpdk-devbind.py --unbind <lspci-address-u-plane-vf>
sudo modprobe vfio_pci
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci <lspci-address-c-plane-vf>
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci <lspci-address-u-plane-vf>
```

We recommand to put the above commands into a script file to quickly repeat them.

<details>
<summary>Example script for Benetel 650 with Intel X710 on host</summary>

```console
set -x

sudo ethtool -G eno12409 rx 4096
sudo ethtool -G eno12409 tx 4096
sudo ifconfig eno12409 mtu 9216

sudo modprobe iavf
sudo sh -c 'echo 0 > /sys/class/net/eno12409/device/sriov_numvfs'
sudo sh -c 'echo 2 > /sys/class/net/eno12409/device/sriov_numvfs'
sudo ip link set eno12409 vf 0 mac 00:11:22:33:44:67 vlan 3 qos 0 spoofchk off mtu 9216
sudo ip link set eno12409 vf 1 mac 00:11:22:33:44:66 vlan 3 qos 0 spoofchk off mtu 9216

sudo /usr/local/bin/dpdk-devbind.py --unbind 31:06.0
sudo /usr/local/bin/dpdk-devbind.py --unbind 31:06.1
sudo modprobe vfio-pci
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci 31:06.0
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci 31:06.1
```
</details>


## Configure OAI gNB

**Beware in the following section to let in the range of isolated cores the parameters that should be (i.e. `L1s.L1_rx_thread_core`, `L1s.L1_tx_thread_core`, `RUs.ru_thread_core`, `fhi_72.io_core` and `fhi_72.worker_cores`)**

Sample configuration files for OAI gNB, specific to the manufacturer of the radio unit, are available at:
1. LITE-ON RU: [`gnb.sa.band78.273prb.fhi72.4x4-liteon.conf`](../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.273prb.fhi72.4x4-liteon.conf) (band n78, 273 PRBs, 3.5GHz center freq, 4x4 antenna configuration with 9 bit I/Q samples (compressed) for PUSCH/PDSCH/PRACH, 2-layer DL MIMO, UL SISO)
2. Benetel 650 RU: [`gnb.sa.band78.273prb.fhi72.4x2-benetel650.conf`](../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.273prb.fhi72.4x2-benetel650.conf) (band n78, 273 PRBs, 3.5GHz center freq, 4x2 antenna configuration with 9 bit I/Q samples (compressed) for PUSCH/PDSCH/PRACH, 2-layer DL MIMO, UL SISO)
3. VVDN RU: [`gnb.sa.band77.273prb.fhi72.4x4-vvdn.conf`](../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band77.273prb.fhi72.4x4-vvdn.conf) (band n77, 273 PRBs, 4.0GHz center freq, 4x4 antenna configuration with 9 bit I/Q samples (compressed) for PUSCH/PDSCH/PRACH, 2-layer DL MIMO, UL SISO)

Edit the sample OAI gNB configuration file and check following parameters:

* `gNBs` section
  * The PLMN section shall match the one defined in the AMF
  * `amf_ip_address` shall be the correct AMF IP address in your system
  * `GNB_INTERFACE_NAME_FOR_NG_AMF` and `GNB_IPV4_ADDRESS_FOR_NG_AMF` shall match your DU N2 interface name and IP address
  * `GNB_INTERFACE_NAME_FOR_NGU` and `GNB_IPV4_ADDRESS_FOR_NGU` shall match your DU N3 interface name and IP address
  * `prach_ConfigurationIndex`
  * `prach_msg1_FrequencyStart`
  * Adjust the frequency, bandwidth and SSB position

* `L1s` section
  * Set an isolated core for L1 thread `L1_rx_thread_core`, in our environment we are using CPU 8
  * Set an isolated core for L1 thread `L1_tx_thread_core`, in our environment we are using CPU 10
  * `phase_compensation` should be set to 0 to disable when it is performed in the RU and set to 1 when it should be performed on the DU side

* `RUs` section
  * Set an isolated core for RU thread `ru_thread_core`, in our environment we are using CPU 6

* `fhi_72` (FrontHaul Interface) section: this config follows the structure
  that is employed by the xRAN library (`xran_fh_init` and `xran_fh_config`
  structs in the code):
  * `dpdk_devices`: PCI addresses of NIC VFs binded to the DPDK (not the physical NIC but the VFs, use `lspci | grep Virtual`)
  * `system_core`: absolute CPU core ID for DPDK control threads
    (`rte_mp_handle`, `eal-intr-thread`, `iavf-event-thread`)
  * `io_core`: absolute CPU core ID for XRAN library, it should be an isolated core, in our environment we are using CPU 4
  * `worker_cores`: array of absolute CPU core IDs for XRAN library, they should be isolated cores, in our environment we are using CPU 2
  * `du_addr`: DU C- and U-plane MAC-addresses (format `UU:VV:WW:XX:YY:ZZ`,
    hexadecimal numbers)
  * `ru_addr`: RU C- and U-plane MAC-addresses (format `UU:VV:WW:XX:YY:ZZ`,
    hexadecimal numbers)
  * `mtu`: Maximum Transmission Unit for the RU, specified by RU vendor
  * `fh_config`: parameters that need to match RU parameters
    * timing parameters (starting with `T`) depend on the RU: `Tadv_cp_dl` is a
      single number, the rest pairs of numbers `(x, y)` specifying minimum and
      maximum delays
    * `ru_config`: RU-specific configuration:
      * `iq_width`: Width of DL/UL IQ samples: if 16, no compression, if <16, applies
        compression
      * `iq_width_prach`: Width of PRACH IQ samples: if 16, no compression, if <16, applies
        compression
      * `fft_size`: size of FFT performed by RU, set to 12 by default
    * `prach_config`: PRACH-specific configuration
      * `eAxC_offset`:  PRACH antenna offset
      * `kbar`: the PRACH guard interval, provided in RU

Layer mapping (eAxC offsets) happens as follows:
- For PUSCH/PDSCH, the layers are mapped to `[0,1,...,N-1]` where `N` is the
  respective RX/TX number of antennas.
- For PRACH, the layers are mapped to `[No,No+1,...No+N-1]` where No is the
  `fhi_72.fh_config.[0].prach_config.eAxC_offset` and `N` the number of receive
  antennas.

xRAN SRS reception is not supported.

# Start and Operation of OAI gNB

Run the `nr-softmodem` from the build directory:
```bash
cd ~/openairinterface5g/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/oran.fh.band78.fr1.273PRB.conf --sa --reorder-thread-disable 1 --thread-pool <list of non isolated cpus>
```

**Note**: You may run OAI with O-RAN 7.2 Fronthaul without a RU attached (e.g. for benchmarking).
In such case, you would generate artificial traffic by replacing the `--sa` option by the `--phy-test` option.

You have to set the thread pool option to non-isolated CPUs, since the thread
pool is used for L1 processing which should not interfere with DPDK threads.
For example if you have two NUMA nodes in your system (for example 18 CPUs per
socket) and odd cores are non-isolated, then you can put the thread-pool on
`1,3,5,7,9,11,13,15`. On the other hand, if you have one NUMA node, you can use
either isolated cores or non isolated cores, but make sure that isolated cores
are not the ones defined earlier for DPDK/xran.

<details>
<summary>Once the gNB runs, you should see counters for PDSCH/PUSCH/PRACH per
antenna port, as follows (4x2 configuration):</summary>

```
[NR_PHY]   [o-du 0][rx   24604 pps   24520 kbps  455611][tx  126652 pps  126092 kbps 2250645][Total Msgs_Rcvd 24604]
[NR_PHY]   [o_du0][pusch0   10766 prach0    1536]
[NR_PHY]   [o_du0][pusch1   10766 prach1    1536]
```

</details>

The first line show RX/TX packet counters, i.e., packets received from the RU
(RX), and sent to the RU (TX). In the second and third line, it shows the
counters for the PUSCH and PRACH ports (2 receive antennas, so two counters
each). These numbers should be equal, otherwise it indicates that you don't
receive enough packets on either port.

<details>
<summary>If you see many zeroes, then it means that OAI does not receive
packets on the fronthaul from the RU (RX is almost 0, all PUSCH/PRACH counters
are 0).</summary>

```
[NR_PHY]   [o-du 0][rx       2 pps       0 kbps       0][tx 1020100 pps  127488 kbps 4717971][Total Msgs_Rcvd 2]
[NR_PHY]   [o_du0][pusch0       0 prach0       0]
[NR_PHY]   [o_du0][pusch1       0 prach1       0]
[NR_PHY]   [o_du0][pusch2       0 prach2       0]
[NR_PHY]   [o_du0][pusch3       0 prach3       0]
```

</details>

In this case, please make sure that the O-RU has been configured with the right
ethernet address of the gNB, and has been activated. You might enable port
mirroring at your switch to capture the fronthaul packets: check that you see
(1) packets at all (2) they have the right ethernet address (3) the right VLAN
tag. Although we did not test this, you might make use of the [DPDK packet
capture feature](https://doc.dpdk.org/guides/howto/packet_capture_framework.html)

<details>
<summary>If you see messages about `Received time doesn't correspond to the
time we think it is` or `Jump in frame counter`, the S-plane is not working.</summary>

```
[PHY]   Received Time doesn't correspond to the time we think it is (slot mismatch, received 480.5, expected 475.8)
[PHY]   Received Time doesn't correspond to the time we think it is (frame mismatch, 480.5 , expected 475.5)
[PHY]   Jump in frame counter last_frame 480 => 519, slot 19
[PHY]   Received Time doesn't correspond to the time we think it is (slot mismatch, received 519.19, expected 480.12)
[PHY]   Received Time doesn't correspond to the time we think it is (frame mismatch, 519.19 , expected 480.19)
[PHY]   Received Time doesn't correspond to the time we think it is (slot mismatch, received 520.1, expected 520.0)
```

You can see that the frame numbers jump around, by 5-40 frames (corresponding
to 50-400ms!). This indicates the gNB receives packets on the fronthaul that
don't match its internal time, and the synchronization between gNB and RU is
not working!

</details>

In this case, you should reverify that `ptp4l` and `phc2sys` are working, e.g.,
do not do any jumps (during the last hour). While an occasional jump is not
necessarily problematic for the gNB, many such messages mean that the system is
not working, and UEs might not be able to attach or reach good performance.
