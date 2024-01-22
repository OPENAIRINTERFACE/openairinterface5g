<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Full Stack 5G-NR L2 simulation with containers and a proxy</font></b>
    </td>
  </tr>
</table>

This page is valid for an `Ubuntu18` host.
It is also valid for `Ubuntu 20.04`.

This tutorial is only valid once this file is merged into the `develop` branch.
# 1. Adapt the `docker-compose` to your environment #
The yaml file is in the following directory:  `ci-scripts/yaml_files/5g_l2sim_tdd`

In the `SMF` section, provide your own DNS IP address:

```yaml
            DEFAULT_DNS_IPV4_ADDRESS=172.21.3.100
```
gNB section:

All the parameter values will be taken from the mounted config file:

```yaml
oai-gnb:
        [...]
        volumes:
            - ../../conf_files/gnb.sa.band78.106prb.l2sim.conf:/opt/oai-gnb/etc/gnb.conf
```

You need to provide your docker-host primary IP address and interface name: in our case `172.21.16.128` and `eno1`.
The following refers to the contents of the previously mentioned .conf file

```yaml
            GNB_NGA_IF_NAME: eno1
            GNB_NGA_IP_ADDRESS: 172.21.16.128
            GNB_NGU_IF_NAME: eno1
            GNB_NGU_IP_ADDRESS: 172.21.16.128
```

For the UE, all the parameter values will be taken from the mounted config
file, in the section `oai-nr-ue0`:

```yaml
  oai-nr-ue0:
        [...]
        volumes:
            - ../../conf_files/nrue.band78.106prb.l2sim.conf:/opt/oai-nr-ue/etc/nr-ue.conf
            - ../../../openair1/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN/AWGN_results:/opt/oai-nr-ue/openair1/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN/AWGN_results
```

In this section, you need to set the proper values for `local_n_if_name` and `remote_n_address`
where the UE configuration (mounted to `/opt/oai-nr-ue/etc/nr-ue.conf`) reads:

```libconfig
MACRLCs = (
        {
        num_cc = 1;
        tr_n_preference = "nfapi";
        local_n_if_name  = "eno1";
        remote_n_address = "127.0.0.1"; //Proxy IP
        local_n_address  = "127.0.0.1";
        ...
```

This tutorial is a first draft. This nFAPI feature and the proxy are still under development.

At time of writing, we were able to run in `host-mode`, 1 `NR-UE` and just ping traffic.




# 2. Retrieving the images on Docker-Hub #

Currently the images are hosted under the team account `oaisoftwarealliance`. They were previously hosted under the user account `rdefosseoai`.

Once again you may need to log on [docker-hub](https://hub.docker.com/) if your organization has reached pulling limit as `anonymous`.

```bash
docker login
```
```bash
Login with your Docker ID to push and pull images from Docker Hub. If you don't have a Docker ID, head over to https://hub.docker.com to create one.
Username:
Password:
```

Now pull images.

```bash
docker pull mysql:8.0
docker pull oaisoftwarealliance/oai-amf:v2.0.0
docker pull oaisoftwarealliance/oai-smf:v2.0.0
docker pull oaisoftwarealliance/oai-upf:v2.0.0
docker pull oaisoftwarealliance/trf-gen-cn5g:focal

docker pull oaisoftwarealliance/oai-gnb:develop
docker pull oaisoftwarealliance/oai-nr-ue:develop
docker pull oaisoftwarealliance/proxy:develop
```

And **re-tag** them for tutorials' docker-compose file to work.

```bash
docker image tag oaisoftwarealliance/proxy:develop oai-lte-multi-ue-proxy:latest
```

```bash
docker logout
```

Note that the proxy image is based on the source available at [https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy](https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy).

At time of writing, the `latest` tag corresponded to `56cfdc046a5f96d5e67d42a2fc2bf6ba2fe58b41` commit.

# 3. Deploy containers #

**CAUTION: this SHALL be done in multiple steps.**

**Just `docker-compose up -d` WILL NOT WORK!**

All the following commands **SHALL** be run from the `ci-scripts/yaml_files/5g_l2sim_tdd` folder.

The `gNB`, `proxy` and `NR-UE` containers will be deployed in `host`-mode. It will use the host loopback interface to connect.

```bash
sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up
```

## 3.1. Deploy OAI 5G Core Network ##

```bash
cd ci-scripts/yaml_files/5g_l2sim_tdd
docker-compose up -d mysql oai-amf oai-smf oai-upf oai-ext-dn
```
```bash
Creating network "l2sim-oai-public-net" with driver "bridge"
Creating network "l2sim-oai-traffic_net-net" with driver "bridge"
Creating l2sim-mysql      ... done
Creating l2sim-oai-upf   ... done
Creating l2sim-oai-amf   ... done
Creating l2sim-oai-smf   ... done
Creating l2sim-oai-ext-dn ... done
```

Wait for a bit.

```bash
docker-compose ps -a
```
```bash
       Name                     Command                  State                  Ports
-------------------------------------------------------------------------------------------------
l2sim-mysql        docker-entrypoint.sh mysqld      Up (healthy)   3306/tcp, 33060/tcp
l2sim-oai-amf      /bin/bash /openair-amf/bin ...   Up (healthy)   38412/sctp, 80/tcp, 9090/tcp
l2sim-oai-ext-dn   /bin/bash -c  apt update;  ...   Up (healthy)
l2sim-oai-smf      /bin/bash /openair-smf/bin ...   Up (healthy)   80/tcp, 8805/udp, 9090/tcp
l2sim-oai-upf      /bin/bash /openair-upf/bin ...   Up (healthy)   2152/udp, 8805/udp
```

At this point, you can prepare a capture on the newly-created public docker bridges:

```bash
ifconfig
```
```bash
...
l2sim-public: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.71.129  netmask 255.255.255.192  broadcast 192.168.71.191
        inet6 fe80::42:c4ff:fe2b:3d38  prefixlen 64  scopeid 0x20<link>
        ether 02:42:c4:2b:3d:38  txqueuelen 0  (Ethernet)
        RX packets 4  bytes 112 (112.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 7  bytes 626 (626.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

l2sim-traffic: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.72.129  netmask 255.255.255.192  broadcast 192.168.72.191
        inet6 fe80::42:b5ff:fed3:e732  prefixlen 64  scopeid 0x20<link>
        ether 02:42:b5:d3:e7:32  txqueuelen 0  (Ethernet)
        RX packets 2652  bytes 142335 (142.3 KB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 3999  bytes 23367972 (23.3 MB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
...
```
```bash
sudo nohup tshark -f "(host 192.168.72.135 and icmp) or (not host 192.168.72.135 and not arp and not port 53 and not port 2152 and not port 2153)" -i l2sim-public -i l2sim-traffic -w /tmp/capture_5g_l2sim_tdd.pcap > /tmp/tshark.log 2>&1 &
```

## 3.2. Deploy OAI gNB in Standalone Mode as a VNF ##

**CAUTION: To execute this 2nd step, the whole `CN5G` SHALL be in `healthy` state (especially the `mysql` container).**

```bash
docker-compose up -d oai-gnb
```
```bash
l2sim-oai-upf is up-to-date
l2sim-oai-ext-dn is up-to-date
Creating l2sim-oai-gnb ... done
```

Wait for a bit.

```bash
$ docker-compose ps -a
```
```bash
       Name                     Command                  State                  Ports
-------------------------------------------------------------------------------------------------
l2sim-mysql        docker-entrypoint.sh mysqld      Up (healthy)   3306/tcp, 33060/tcp
l2sim-oai-amf      /bin/bash /openair-amf/bin ...   Up (healthy)   38412/sctp, 80/tcp, 9090/tcp
l2sim-oai-ext-dn   /bin/bash -c  apt update;  ...   Up (healthy)
l2sim-oai-gnb      /opt/oai-gnb/bin/entrypoin ...   Up (healthy)
l2sim-oai-smf      /bin/bash /openair-smf/bin ...   Up (healthy)   80/tcp, 8805/udp, 9090/tcp
l2sim-oai-upf      /bin/bash /openair-upf/bin ...   Up (healthy)   2152/udp, 8805/udp
```

You can verify that the `gNB` is connected with the `AMF`:

```bash
docker logs l2sim-oai-amf
```
```bash
...
[AMF] [amf_app] [info ] |----------------------------------------------------gNBs' information-------------------------------------------|
[AMF] [amf_app] [info ] |    Index    |      Status      |       Global ID       |       gNB Name       |               PLMN             |
[AMF] [amf_app] [info ] |      1      |    Connected     |         0x0       |         gnb-l2sim-vnf    |            208, 99             |
[AMF] [amf_app] [info ] |----------------------------------------------------------------------------------------------------------------|
...
```

## 3.3. Deploy OAI NR-UE and proxy

```bash
docker-compose up -d proxy oai-nr-ue
```
```bash
l2sim-mysql is up-to-date
l2sim-oai-amf is up-to-date
l2sim-oai-smf is up-to-date
l2sim-oai-upf is up-to-date
l2sim-oai-ext-dn is up-to-date
l2sim-oai-gnb is up-to-date
Creating l2sim-oai-nr-ue ... done
Creating l2sim-proxy ... done
```

Wait for a bit.

```bash
docker-compose ps -a
```
```bash
       Name                     Command                  State                  Ports
-------------------------------------------------------------------------------------------------
l2sim-mysql        docker-entrypoint.sh mysqld      Up (healthy)   3306/tcp, 33060/tcp
l2sim-oai-amf      /bin/bash /openair-amf/bin ...   Up (healthy)   38412/sctp, 80/tcp, 9090/tcp
l2sim-oai-ext-dn   /bin/bash -c  apt update;  ...   Up (healthy)
l2sim-oai-gnb      /opt/oai-gnb/bin/entrypoin ...   Up (healthy)
l2sim-oai-nr-ue0   /opt/oai-nr-ue/bin/entrypo ...   Up (healthy)
l2sim-oai-smf      /bin/bash /openair-smf/bin ...   Up (healthy)   80/tcp, 8805/udp, 9090/tcp
l2sim-oai-upf      /bin/bash /openair-upf/bin ...   Up (healthy)   2152/udp, 8805/udp
l2sim-proxy        /oai-lte-multi-ue-proxy/bi ...   Up (healthy)
```
```bash
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}	{{.MemUsage}}\t{{.MemPerc}}" l2sim-mysql l2sim-oai-amf l2sim-oai-ext-dn l2sim-oai-gnb l2sim-oai-nr-ue0 l2sim-oai-smf l2sim-oai-upf l2sim-proxy
```
```bash
CONTAINER          CPU %     MEM USAGE / LIMIT     MEM %
l2sim-mysql        0.03%     206.7MiB / 62.54GiB   0.32%
l2sim-oai-amf      4.05%     29.49MiB / 62.54GiB   0.05%
l2sim-oai-ext-dn   0.00%     31.27MiB / 62.54GiB   0.05%
l2sim-oai-gnb      1.29%     1.853GiB / 62.54GiB   2.96%
l2sim-oai-nr-ue0   1.43%     350.8MiB / 62.54GiB   0.55%
l2sim-oai-smf      3.24%     30.23MiB / 62.54GiB   0.05%
l2sim-oai-upf      0.00%     11.78MiB / 62.54GiB   0.02%
l2sim-proxy        6.97%     290.4MiB / 62.54GiB   0.45%
```

**CAUTION: As you can see the CPU usage is not that important compared to a 5G RF simulator equivalent.**

But the CPU speed matters; I am running on a fast server:

```bash
lscpu
```
```bash
Architecture:        x86_64
CPU(s):              16
...
Model name:          Intel(R) Xeon(R) Silver 4215R CPU @ 3.20GHz
```

I tried on a slower server with more CPUs and it did not work:

```bash
oaici@orion:~$ lscpu
Architecture:        x86_64
CPU(s):              48
...
Model name:          Intel(R) Xeon(R) CPU E5-2658 v3 @ 2.20GHz
```

We will work on this issue.


Making sure the OAI UE is connected:

```bash
docker exec -it l2sim-oai-nr-ue0 /bin/bash
```
```bash
root@bb4d400a832d:/opt/oai-nr-ue# ifconfig
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.71.137  netmask 255.255.255.192  broadcast 192.168.71.191
        ether 02:42:c0:a8:47:89  txqueuelen 0  (Ethernet)
        RX packets 224259  bytes 5821372018 (5.8 GB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 235916  bytes 7848786376 (7.8 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

oaitun_ue1: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 12.1.1.2  netmask 255.255.255.0  destination 12.1.1.2
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

# 4. Check traffic #

## 4.1. Check your Internet connectivity ##

You can also check with the `ext-dn` container (IP address is `192.168.72.135` in docker-compose)

```bash
docker exec -it l2sim-oai-nr-ue0 /bin/bash
```
```bash
ping -I oaitun_ue1 -c 20 192.168.72.135
```
```bash
root@bb4d400a832d# ping -I oaitun_ue1 -c 20 192.168.72.135
PING 192.168.72.135 (192.168.72.135) from 12.1.1.2 oaitun_ue1: 56(84) bytes of data.
64 bytes from 192.168.72.135: icmp_seq=1 ttl=63 time=65.1 ms
64 bytes from 192.168.72.135: icmp_seq=2 ttl=63 time=74.2 ms
...
64 bytes from 192.168.72.135: icmp_seq=19 ttl=63 time=25.0 ms
64 bytes from 192.168.72.135: icmp_seq=20 ttl=63 time=34.7 ms

--- 192.168.72.135 ping statistics ---
20 packets transmitted, 20 received, 0% packet loss, time 19025ms
rtt min/avg/max/mdev = 16.413/120.639/209.673/57.159 ms
```

# 5. Un-deployment #

```bash
docker-compose down
```
```bash
Stopping l2sim-oai-nr-ue2 ... done
Stopping l2sim-oai-nr-ue  ... done
Stopping l2sim-oai-gnb    ... done
Stopping l2sim-oai-ext-dn ... done
Stopping l2sim-oai-upf    ... done
Stopping l2sim-oai-smf    ... done
Stopping l2sim-oai-amf    ... done
Stopping l2sim-mysql      ... done
Removing l2sim-oai-nr-ue2 ... done
Removing l2sim-oai-nr-ue  ... done
Removing l2sim-oai-gnb    ... done
Removing l2sim-oai-ext-dn ... done
Removing l2sim-oai-upf    ... done
Removing l2sim-oai-smf    ... done
Removing l2sim-oai-amf    ... done
Removing l2sim-mysql      ... done
Removing network l2sim-oai-public-net
Removing network l2sim-oai-traffic-net
```

