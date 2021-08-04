<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Full Stack RF simulation with containers</font></b>
    </td>
  </tr>
</table>

This page is only valid for an `Ubuntu18` host.

**TABLE OF CONTENTS**

1. [Retrieving the images on Docker-Hub](#1-retrieving-the-images-on-docker-hub)
2. [Deploy containers](#2-deploy-containers)
   1. [Deploy and Configure Cassandra Database](#21-deploy-and-configure-cassandra-database)
   2. [Deploy OAI CN4G containers](#22-deploy-oai-cn4g-containers)
   3. [Deploy OAI eNB in RF simulator mode](#23-deploy-oai-enb-in-rf-simulator-mode)
   4. [Deploy OAI LTE UE in RF simulator mode](#24-deploy-oai-lte-ue-in-rf-simulator-mode)
3. [Check traffic](#3-check-traffic)
4. [Un-deployment](#4-un-deployment)
5. [Explanation on the configuration](#5-explanation-on-the-configuration)
   1. [UE IMSI and Keys](#51-ue-imsi-and-keys)
   2. [PLMN and TAI](#52-plmn-and-tai)
   3. [Access to Internet](#53-access-to-internet)

# 1. Retrieving the images on Docker-Hub #

Currently the images are hosted under the user account `rdefosseoai`.

This may change in the future.

Once again you may need to log on [docker-hub](https://hub.docker.com/) if your organization has reached pulling limit as `anonymous`.

```bash
$ docker login
Login with your Docker ID to push and pull images from Docker Hub. If you don't have a Docker ID, head over to https://hub.docker.com to create one.
Username:
Password:
```

Now pull images.

```bash
$ docker pull cassandra:2.1
$ docker pull rdefosseoai/oai-hss:latest
$ docker pull rdefosseoai/oai-mme:latest
$ docker pull rdefosseoai/oai-spgwc:latest
$ docker pull rdefosseoai/oai-spgwu-tiny:latest
```

And **re-tag** them for tutorials' docker-compose file to work.

```bash
$ docker image tag rdefosseoai/oai-spgwc:latest oai-spgwc:latest
$ docker image tag rdefosseoai/oai-hss:latest oai-hss:latest
$ docker image tag rdefosseoai/oai-spgwu-tiny:latest oai-spgwu-tiny:latest 
$ docker image tag rdefosseoai/oai-mme:latest oai-mme:latest
```

```bash
$ docker logout
```

How to build the Traffic-Generator image is explained [here](https://github.com/OPENAIRINTERFACE/openair-epc-fed/blob/master/docs/GENERATE_TRAFFIC.md#1-build-a-traffic-generator-image).

I will soon push also RAN images.

# 2. Deploy containers #

**CAUTION: this SHALL be done in multiple steps.**

**Just `docker-compose up -d` WILL NOT WORK!**

## 2.1. Deploy and Configure Cassandra Database ##

It is very crutial that the Cassandra DB is fully configured before you do anything else!

```bash
$ docker-compose up -d db_init
Creating network "rfsim4g-oai-private-net" with the default driver
Creating network "rfsim4g-oai-public-net" with the default driver
Creating rfsim4g-cassandra ... done
Creating rfsim4g-db-init   ... done

$ docker logs rfsim4g-db-init --follow
Connection error: ('Unable to connect to any servers', {'192.168.68.2': error(111, "Tried connecting to [('192.168.68.2', 9042)]. Last error: Connection refused")})
...
Connection error: ('Unable to connect to any servers', {'192.168.68.2': error(111, "Tried connecting to [('192.168.68.2', 9042)]. Last error: Connection refused")})
OK
```

**You SHALL wait until you HAVE the `OK` message in the logs!**

```bash
$ docker rm rfsim4g-db-init
```

## 2.2. Deploy OAI CN4G containers ##

```bash
$ docker-compose up -d oai_mme oai_spgwu trf_gen
rfsim4g-cassandra is up-to-date
Creating rfsim4g-trf-gen   ... done
Creating rfsim4g-oai-hss ... done
Creating rfsim4g-oai-mme ... done
Creating rfsim4g-oai-spgwc ... done
Creating rfsim4g-oai-spgwu-tiny ... done
```

You shall wait until all containers are `healthy`. About 10 seconds!

```bash
$ docker-compose ps -a
         Name                       Command                  State                            Ports                      
-------------------------------------------------------------------------------------------------------------------------
rfsim4g-cassandra        docker-entrypoint.sh cassa ...   Up (healthy)   7000/tcp, 7001/tcp, 7199/tcp, 9042/tcp, 9160/tcp
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp          
rfsim4g-oai-mme          /openair-mme/bin/entrypoin ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp                    
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp                              
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp                              
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)                                                   
```

## 2.3. Deploy OAI eNB in RF simulator mode ##

```bash
$ docker-compose up -d enb
Creating rfsim4g-oai-enb ... done
```

Again wait for the healthy state:

```bash
$ docker-compose ps -a
         Name                       Command                  State                            Ports                      
-------------------------------------------------------------------------------------------------------------------------
rfsim4g-cassandra        docker-entrypoint.sh cassa ...   Up (healthy)   7000/tcp, 7001/tcp, 7199/tcp, 9042/tcp, 9160/tcp
rfsim4g-oai-enb          /opt/oai-enb/bin/entrypoin ...   Up (healthy)   2152/udp, 36412/udp, 36422/udp                  
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp          
rfsim4g-oai-mme          /openair-mme/bin/entrypoin ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp                    
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp                              
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp                              
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)                                    
```

Check if the eNB connected to MME:

```bash
$ docker logs rfsim4g-oai-mme
...
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0039    ======================================= STATISTICS ============================================

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0042                   |   Current Status| Added since last display|  Removed since last display |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0048    Connected eNBs |          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0054    Attached UEs   |          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0060    Connected UEs  |          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0066    Default Bearers|          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0072    S1-U Bearers   |          0      |              0              |             0               |

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0075    ======================================= STATISTICS ============================================

DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0469    Client association changed: 0
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0101    ----------------------
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0102    SCTP Status:
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0103    assoc id .....: 675
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0104    state ........: 4
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0105    instrms ......: 2
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0106    outstrms .....: 2
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0108    fragmentation : 1452
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0109    pending data .: 0
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0110    unack data ...: 0
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0111    rwnd .........: 106496
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0112    peer info     :
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0114        state ....: 2
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0116        cwnd .....: 4380
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0118        srtt .....: 0
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0120        rto ......: 3000
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0122        mtu ......: 1500
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0123    ----------------------
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0479    New connection
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0205    ----------------------
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0206    Local addresses:
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0217        - [192.168.61.3]
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0234    ----------------------
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0151    ----------------------
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0152    Peer addresses:
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0163        - [192.168.61.20]
DEBUG SCTP   enair-mme/src/sctp/sctp_common.c:0178    ----------------------
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0554    SCTP RETURNING!!
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0547    [675][44] Msg of length 51 received from port 36412, on stream 0, PPID 18
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0554    SCTP RETURNING!!
DEBUG S1AP   mme/src/s1ap/s1ap_mme_handlers.c:2826    Create eNB context for assoc_id: 675
DEBUG S1AP   mme/src/s1ap/s1ap_mme_handlers.c:0361    S1-Setup-Request macroENB_ID.size 3 (should be 20)
DEBUG S1AP   mme/src/s1ap/s1ap_mme_handlers.c:0321    New s1 setup request incoming from macro eNB id: 00e01
DEBUG S1AP   mme/src/s1ap/s1ap_mme_handlers.c:0423    Adding eNB to the list of served eNBs
DEBUG S1AP   mme/src/s1ap/s1ap_mme_handlers.c:0438    Adding eNB id 3585 to the list of served eNBs
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0283    [44][675] Sending buffer 0x7f9394009f90 of 27 bytes on stream 0 with ppid 18
DEBUG SCTP   rc/sctp/sctp_primitives_server.c:0296    Successfully sent 27 bytes on stream 0
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0039    ======================================= STATISTICS ============================================

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0042                   |   Current Status| Added since last display|  Removed since last display |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0048    Connected eNBs |          1      |              1              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0054    Attached UEs   |          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0060    Connected UEs  |          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0066    Default Bearers|          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0072    S1-U Bearers   |          0      |              0              |             0               |

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0075    ======================================= STATISTICS ============================================
...
```

## 2.4. Deploy OAI LTE UE in RF simulator mode ##

```bash
$ docker-compose up -d oai_ue0
Creating rfsim4g-oai-lte-ue0 ... done
```

Again a bit of patience:

```bash
$ docker-compose ps -a
         Name                       Command                  State                            Ports                      
-------------------------------------------------------------------------------------------------------------------------
rfsim4g-cassandra        docker-entrypoint.sh cassa ...   Up (healthy)   7000/tcp, 7001/tcp, 7199/tcp, 9042/tcp, 9160/tcp
rfsim4g-oai-enb          /opt/oai-enb/bin/entrypoin ...   Up (healthy)   2152/udp, 36412/udp, 36422/udp                  
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp          
rfsim4g-oai-lte-ue0      /opt/oai-lte-ue/bin/entryp ...   Up (healthy)   10000/tcp                                       
rfsim4g-oai-mme          /openair-mme/bin/entrypoin ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp                    
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp                              
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp                              
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)                                             
Creating rfsim4g-oai-enb ... done
```

Making sure the OAI UE is connected:

```bash
$ docker logs rfsim4g-oai-enb
...
[RRC]   RRCConnectionReconfiguration Encoded 1098 bits (138 bytes)
[RRC]   [eNB 0] Frame 0, Logical Channel DL-DCCH, Generate LTE_RRCConnectionReconfiguration (bytes 138, UE id 617b)
[RRC]   sent RRC_DCCH_DATA_REQ to TASK_PDCP_ENB
[PDCP]   [FRAME 00000][eNB][MOD 00][RNTI 617b][SRB 02]  Action ADD  LCID 2 (SRB id 2) configured with SN size 5 bits and RLC AM
[PDCP]   [FRAME 00000][eNB][MOD 00][RNTI 617b][DRB 01]  Action ADD  LCID 3 (DRB id 1) configured with SN size 12 bits and RLC AM
[SCTP]   Successfully sent 46 bytes on stream 1 for assoc_id 676
[RRC]   [FRAME 00000][eNB][MOD 00][RNTI 617b] UE State = RRC_RECONFIGURED (default DRB, xid 0)
[PDCP]   [FRAME 00000][eNB][MOD 00][RNTI 617b][SRB 02]  Action MODIFY LCID 2 RB id 2 reconfigured with SN size 5 and RLC AM 
[PDCP]   [FRAME 00000][eNB][MOD 00][RNTI 617b][DRB 01]  Action MODIFY LCID 3 RB id 1 reconfigured with SN size 1 and RLC AM 
[RRC]   [eNB 0] Frame  0 CC 0 : SRB2 is now active
[RRC]   [eNB 0] Frame  0 : Logical Channel UL-DCCH, Received LTE_RRCConnectionReconfigurationComplete from UE rnti 617b, reconfiguring DRB 1/LCID 3
[RRC]   [eNB 0] Frame  0 : Logical Channel UL-DCCH, Received LTE_RRCConnectionReconfigurationComplete, reconfiguring DRB 1/LCID 3
[MAC]   UE 0 RNTI 617b adding LC 3 idx 2 to scheduling control (total 3)
[MAC]   Added physicalConfigDedicated 0x7f98e0004950 for 0.0
[S1AP]   initial_ctxt_resp_p: e_rab ID 5, enb_addr 192.168.61.20, SIZE 4 
[SCTP]   Successfully sent 40 bytes on stream 1 for assoc_id 676
[SCTP]   Successfully sent 61 bytes on stream 1 for assoc_id 676
...
```

On the MME:

```bash
$ docker logs rfsim4g-oai-mme
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0039    ======================================= STATISTICS ============================================

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0042                   |   Current Status| Added since last display|  Removed since last display |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0048    Connected eNBs |          1      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0054    Attached UEs   |          1      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0060    Connected UEs  |          1      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0066    Default Bearers|          0      |              0              |             0               |
DEBUG MME-AP src/mme_app/mme_app_statistics.c:0072    S1-U Bearers   |          0      |              0              |             0               |

DEBUG MME-AP src/mme_app/mme_app_statistics.c:0075    ======================================= STATISTICS ============================================
```

On the LTE UE:

```bash
$ docker exec rfsim4g-oai-lte-ue0 /bin/bash -c "ifconfig"
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.61.30  netmask 255.255.255.192  broadcast 192.168.61.63
        ether 02:42:c0:a8:3d:1e  txqueuelen 0  (Ethernet)
        RX packets 1109931  bytes 8078031934 (8.0 GB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 1232068  bytes 7798928848 (7.7 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

oaitun_ue1: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 12.0.0.2  netmask 255.0.0.0  destination 12.0.0.2
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

oaitun_uem1: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 10.0.2.2  netmask 255.255.255.0  destination 10.0.2.2
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

The tunnel `oaitun_ue1` SHALL be mounted and with an IP address in the `12.0.0.xxx` range.

# 3. Check traffic #

```bash
$ docker exec rfsim4g-oai-lte-ue0 /bin/bash -c "ping -c 2 www.lemonde.fr"
PING s2.shared.global.fastly.net (151.101.122.217) 56(84) bytes of data.
64 bytes from 151.101.122.217 (151.101.122.217): icmp_seq=1 ttl=54 time=12.9 ms
64 bytes from 151.101.122.217 (151.101.122.217): icmp_seq=2 ttl=54 time=12.9 ms

--- s2.shared.global.fastly.net ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1001ms
rtt min/avg/max/mdev = 12.940/12.965/12.990/0.025 ms

$ docker exec rfsim4g-oai-lte-ue0 /bin/bash -c "ping -I oaitun_ue1 -c 2 www.lemonde.fr"
PING s2.shared.global.fastly.net (151.101.122.217) from 12.0.0.2 oaitun_ue1: 56(84) bytes of data.
64 bytes from 151.101.122.217 (151.101.122.217): icmp_seq=1 ttl=53 time=23.6 ms
64 bytes from 151.101.122.217 (151.101.122.217): icmp_seq=2 ttl=53 time=29.5 ms

--- s2.shared.global.fastly.net ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1001ms
rtt min/avg/max/mdev = 23.659/26.626/29.593/2.967 ms
```

The 1st ping command is NOT using the OAI stack. My network infrastructure has a response of `13 ms` to reach this website.

The 2nd ping command is using the OAI stack. So the stack takes `26.6 - 12.9 = 13.7 ms`.

# 4. Un-deployment #

```bash
$ docker-compose down
Stopping rfsim4g-oai-lte-ue0    ... done
Stopping rfsim4g-oai-enb        ... done
Stopping rfsim4g-oai-spgwu-tiny ... done
Stopping rfsim4g-oai-spgwc      ... done
Stopping rfsim4g-oai-mme        ... done
Stopping rfsim4g-oai-hss        ... done
Stopping rfsim4g-trf-gen        ... done
Stopping rfsim4g-cassandra      ... done
Removing rfsim4g-oai-lte-ue0    ... done
Removing rfsim4g-oai-enb        ... done
Removing rfsim4g-oai-spgwu-tiny ... done
Removing rfsim4g-oai-spgwc      ... done
Removing rfsim4g-oai-mme        ... done
Removing rfsim4g-oai-hss        ... done
Removing rfsim4g-trf-gen        ... done
Removing rfsim4g-cassandra      ... done
Removing network rfsim4g-oai-private-net
Removing network rfsim4g-oai-public-net
```

# 5. Explanation on the configuration #

With a single `docker-compose.yml` file, it is easier to explain how I made the full connection.

Try to modify as little as possible. And if you don't understand a field/value, you'd better NOT modify it.

## 5.1. UE IMSI and Keys ##

in HSS config:

```yaml
            OP_KEY: 1006020f0a478bf6b699f15c062e42b3
            LTE_K: fec86ba6eb707ed08905757b1bb44b8f
            APN1: oai.ipv4
            APN2: internet
            FIRST_IMSI: 208960100000001
            NB_USERS: 10
```

in UE config:

```yaml
            MCC: '208'
            MNC: '96'
            SHORT_IMSI: '0100000001'
            LTE_KEY: 'fec86ba6eb707ed08905757b1bb44b8f'
            OPC: 'c42449363bbad02b66d16bc975d77cc1'
            MSISDN: '001011234561010'
            HPLMN: 20896
```

As you can see: `LTE_K` and `LTE_KEY` are the same value. And `OP_KEY` and `OPC` can be deduced from each other. Look in HSS logs.

```bash
$ docker logs rfsim4g-oai-hss
...
Compute opc:
	K:  FEC86BA6EB707ED08905757B1BB44B8F           <== `LTE_K`
	In: 1006020F0A478BF6B699F15C062E42B3           <== `OP_KEY`
	Rinj:   D4224B3931FD5BDDD0489A9573F93E72
	Out:    C42449363BBAD02B66D16BC975D77CC1       <== `OPC`
...
```

In HSS, I've provisioned 10 users starting at `208960100000001` (`FIRST_IMSI` and `NB_USERS`).

My 1st UE IMSI is an aggregation of `MCC`, `MNC`, `SHORT_IMSI`.

## 5.2. PLMN and TAI ##

in MME config:

```yaml
            REALM: openairinterface.org
..
            MCC: '208'
            MNC: '96'
            MME_GID: 32768
            MME_CODE: 3
            TAC_0: 1
            TAC_1: 2
            TAC_2: 3
            MME_FQDN: mme.openairinterface.org
```

in SPGW-C/-U configs:

```yaml
            MCC: '208'
            MNC: '96'
            MNC03: '096'
            TAC: 1
            GW_ID: 1
            REALM: openairinterface.org
```

in eNB config:

```yaml
            MCC: '208'
            MNC: '96'
            MNC_LENGTH: 2
            TAC: 1
```

The values SHALL match, and `TAC` shall match `TAC_0` from MME.

## 5.3. Access to Internet ##

In my traffic test, I was able to ping outside of my local network.

in SPGW-C config:

```yaml
            DEFAULT_DNS_IPV4_ADDRESS: 192.168.18.129
            DEFAULT_DNS_SEC_IPV4_ADDRESS: 8.8.4.4
            PUSH_PROTOCOL_OPTION: 'true'
```

in SPGW-U config:

```yaml
            NETWORK_UE_NAT_OPTION: 'yes'
```

Please put your own DNS server IP adress.

And you may have to play with `PUSH_PROTOCOL_OPTION` and `NETWORK_UE_NAT_OPTION` depending on your network.

