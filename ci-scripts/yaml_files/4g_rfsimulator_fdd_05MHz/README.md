<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Full Stack 4G-LTE RF simulation with containers</font></b>
    </td>
  </tr>
</table>

This page is only valid for an `Ubuntu18` host.

**Table of Contents**

[[_TOC_]]

# 1. Retrieving the images on Docker-Hub #

Currently the images are hosted under the team account `oaisoftwarealliance`. They were previously hosted under the user account `rdefosseoai`.

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
$ docker pull redis:6.0.5
$ docker pull oaisoftwarealliance/oai-hss:latest
$ docker pull oaisoftwarealliance/magma-mme:latest
$ docker pull oaisoftwarealliance/oai-spgwc:latest
$ docker pull oaisoftwarealliance/oai-spgwu-tiny:latest

$ docker pull oaisoftwarealliance/oai-enb:develop
$ docker pull oaisoftwarealliance/oai-lte-ue:develop
```

If the `redis` tag is not available, pick the newest available `6.0.x` tag at [Docker Hub Redis Tags](https://hub.docker.com/_/redis?tab=tags).

And **re-tag** them for tutorials' docker-compose file to work.

```bash
$ docker image tag oaisoftwarealliance/oai-spgwc:latest oai-spgwc:latest
$ docker image tag oaisoftwarealliance/oai-hss:latest oai-hss:latest
$ docker image tag oaisoftwarealliance/oai-spgwu-tiny:latest oai-spgwu-tiny:latest 
$ docker image tag oaisoftwarealliance/magma-mme:latest magma-mme:latest
```

```bash
$ docker logout
```

How to build the Traffic-Generator image is explained [here](https://github.com/OPENAIRINTERFACE/openair-epc-fed/blob/master/docs/GENERATE_TRAFFIC.md#1-build-a-traffic-generator-image).

# 2. Deploy containers #

**CAUTION: this SHALL be done in multiple steps.**

**Just `docker-compose up -d` WILL NOT WORK!**

All the following commands **SHALL** be run from the `ci-scripts/yaml_files/4g_rfsimulator_fdd_05MHz` folder.

## 2.1. Deploy and Configure Cassandra Database ##

It is very crutial that the Cassandra DB is fully configured before you do anything else!

```bash
$ cd ci-scripts/yaml_files/4g_rfsimulator_fdd_05MHz
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

At this point, you can prepare a capture on the newly-created public docker bridge:

```bash
$ ifconfig rfsim4g-public
        inet 192.168.61.1  netmask 255.255.255.192  broadcast 192.168.61.63
        ether 02:42:8f:dd:ba:5a  txqueuelen 0  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
$ sudo tshark -i rfsim4g-public -f 'port 3868 or port 2123 or port 36412 or port 36422 or port 46520 or port 8805' -w /tmp/my-oai-control-plane.pcap
```

**BE CAREFUL: please use that filter or you will also capture the data-plane with IQ samples between `eNB` and `LTE-UE`.**

**and your capture WILL become huge (10s of Gbytes).**

## 2.2. Deploy OAI CN4G containers ##

```bash
$ docker-compose up -d magma_mme oai_spgwu trf_gen
rfsim4g-cassandra is up-to-date
Creating rfsim4g-trf-gen ... done
Creating rfsim4g-redis   ... done
Creating rfsim4g-oai-hss ... done
Creating rfsim4g-magma-mme ... done
Creating rfsim4g-oai-spgwc ... done
Creating rfsim4g-oai-spgwu-tiny ... done
```

You shall wait until all containers are `healthy`. About 10 seconds!

```bash
$ docker-compose ps -a
         Name                       Command                  State                            Ports                      
-------------------------------------------------------------------------------------------------------------------------
rfsim4g-cassandra        docker-entrypoint.sh cassa ...   Up (healthy)   7000/tcp, 7001/tcp, 7199/tcp, 9042/tcp, 9160/tcp
rfsim4g-magma-mme        /bin/bash -c /magma-mme/bi ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp
rfsim4g-redis            /bin/bash -c redis-server  ...   Up (healthy)   6379/tcp
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)
```

## 2.3. Deploy OAI eNB in RF simulator mode ##

```bash
$ docker-compose up -d oai_enb0
Creating rfsim4g-oai-enb ... done
```

Again wait for the healthy state:

```bash
$ docker-compose ps -a
         Name                       Command                  State                            Ports                      
-------------------------------------------------------------------------------------------------------------------------
rfsim4g-cassandra        docker-entrypoint.sh cassa ...   Up (healthy)   7000/tcp, 7001/tcp, 7199/tcp, 9042/tcp, 9160/tcp
rfsim4g-magma-mme        /bin/bash -c /magma-mme/bi ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp
rfsim4g-oai-enb          /opt/oai-enb/bin/entrypoin ...   Up (healthy)   2152/udp, 36412/udp, 36422/udp
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp
rfsim4g-redis            /bin/bash -c redis-server  ...   Up (healthy)   6379/tcp
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)
```

Check if the eNB connected to MME: with MAGMA-MME, the logs are not pushed to `stdout` but to a file at `/var/log/mme.log`

```bash
$ docker exec rfsim4g-magma-mme /bin/bash -c "cat /var/log/mme.log"
...
000141 Fri Feb 11 09:49:29 2022 7EFFE7BE4700 DEBUG S6A    tasks/s6a/s6a_peer.c            :0095    Diameter identity of MME: mme.openairinterface.org with length: 24
000142 Fri Feb 11 09:49:29 2022 7EFFE7BE4700 DEBUG S6A    tasks/s6a/s6a_peer.c            :0130    S6a peer connection attempt 1 / 8
000143 Fri Feb 11 09:49:29 2022 7EFFE7BE4700 DEBUG S6A    tasks/s6a/s6a_peer.c            :0141    Peer hss.openairinterface.org is now connected...
000144 Fri Feb 11 09:49:29 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000146 Fri Feb 11 09:49:29 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_main.c    :0083    MME APP ZMQ latency: 214.
000145 Fri Feb 11 09:49:29 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme.c           :0121    S1AP ZMQ latency: 146.
000147 Fri Feb 11 09:50:23 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme.c           :0121    S1AP ZMQ latency: 144.
000148 Fri Feb 11 09:50:23 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme_handlers.c  :3775    Create eNB context for assoc_id: 4121
000149 Fri Feb 11 09:50:23 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme.c           :0121    S1AP ZMQ latency: 48.
000150 Fri Feb 11 09:50:23 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme_handlers.c  :0536    New s1 setup request incoming from eNB-rf-sim macro eNB id: 00e01
000151 Fri Feb 11 09:50:23 2022 7EFFE83E5700 DEBUG S1AP   tasks/s1ap/s1ap_mme_handlers.c  :0639    Adding eNB with enb_id :3585 to the list of served eNBs 
000152 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0049    ======================================= STATISTICS ============================================

000153 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0050                   |   Current Status|
000154 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0053    Attached UEs   |          0      |
000155 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0056    Connected UEs  |          0      |
000156 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0059    Connected eNBs |          0      |
000157 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0062    Default Bearers|          0      |
000158 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0065    S1-U Bearers   |          0      |

000159 Fri Feb 11 09:50:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0070    ======================================= STATISTICS ============================================

000160 Fri Feb 11 09:50:28 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000161 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0049    ======================================= STATISTICS ============================================

000162 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0050                   |   Current Status|
000163 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0053    Attached UEs   |          0      |
000164 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0056    Connected UEs  |          0      |
000165 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0059    Connected eNBs |          1      |
000166 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0062    Default Bearers|          0      |
000167 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0065    S1-U Bearers   |          0      |

000168 Fri Feb 11 09:51:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0070    ======================================= STATISTICS ============================================

000169 Fri Feb 11 09:51:28 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
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
rfsim4g-magma-mme        /bin/bash -c /magma-mme/bi ...   Up (healthy)   2123/udp, 3870/tcp, 5870/tcp
rfsim4g-oai-enb          /opt/oai-enb/bin/entrypoin ...   Up (healthy)   2152/udp, 36412/udp, 36422/udp
rfsim4g-oai-hss          /openair-hss/bin/entrypoin ...   Up (healthy)   5868/tcp, 9042/tcp, 9080/tcp, 9081/tcp
rfsim4g-oai-lte-ue0      /opt/oai-lte-ue/bin/entryp ...   Up (healthy)   10000/tcp
rfsim4g-oai-spgwc        /openair-spgwc/bin/entrypo ...   Up (healthy)   2123/udp, 8805/udp
rfsim4g-oai-spgwu-tiny   /openair-spgwu-tiny/bin/en ...   Up (healthy)   2152/udp, 8805/udp
rfsim4g-redis            /bin/bash -c redis-server  ...   Up (healthy)   6379/tcp
rfsim4g-trf-gen          /bin/bash -c ip route add  ...   Up (healthy)
```

Making sure the OAI UE is connected:

```bash
$ docker logs rfsim4g-oai-enb
...
7320446.555734 [RRC] I [FRAME 00000][eNB][MOD 00][RNTI bd8c] Logical Channel DL-DCCH, Generate UECapabilityEnquiry (bytes 10)
7320446.555739 [RRC] I sent RRC_DCCH_DATA_REQ to TASK_PDCP_ENB
7320446.570284 [RRC] I [FRAME 00000][eNB][MOD 00][RNTI bd8c] received ueCapabilityInformation on UL-DCCH 1 from UE
7320446.570293 [RRC] A got UE capabilities for UE bd8c
7320446.570346 [RRC] W drx_Configuration parameter is NULL, cannot configure local UE parameters or CDRX is deactivated
7320446.570370 [RRC] I [eNB 0] frame 0: requesting A2, A3, A4, and A5 event reporting
7320446.570435 [RRC] I RRCConnectionReconfiguration Encoded 1146 bits (144 bytes)
7320446.570440 [RRC] I [eNB 0] Frame 0, Logical Channel DL-DCCH, Generate LTE_RRCConnectionReconfiguration (bytes 144, UE id bd8c)
7320446.570445 [RRC] I sent RRC_DCCH_DATA_REQ to TASK_PDCP_ENB
7320446.570446 [SCTP] I Successfully sent 46 bytes on stream 1 for assoc_id 4120
7320446.570449 [PDCP] I [FRAME 00000][eNB][MOD 00][RNTI bd8c][SRB 02]  Action ADD  LCID 2 (SRB id 2) configured with SN size 5 bits and RLC AM
7320446.570455 [PDCP] I [FRAME 00000][eNB][MOD 00][RNTI bd8c][DRB 01]  Action ADD  LCID 3 (DRB id 1) configured with SN size 12 bits and RLC AM
7320446.583036 [RRC] I [FRAME 00000][eNB][MOD 00][RNTI bd8c] UE State = RRC_RECONFIGURED (default DRB, xid 0)
7320446.583056 [PDCP] I [FRAME 00000][eNB][MOD 00][RNTI bd8c][SRB 02]  Action MODIFY LCID 2 RB id 2 reconfigured with SN size 5 and RLC AM 
7320446.583060 [PDCP] I [FRAME 00000][eNB][MOD 00][RNTI bd8c][DRB 01]  Action MODIFY LCID 3 RB id 1 reconfigured with SN size 1 and RLC AM 
7320446.583065 [RRC] I [eNB 0] Frame  0 CC 0 : SRB2 is now active
7320446.583067 [RRC] I [eNB 0] Frame  0 : Logical Channel UL-DCCH, Received LTE_RRCConnectionReconfigurationComplete from UE rnti bd8c, reconfiguring DRB 1/LCID 3
7320446.583070 [RRC] I [eNB 0] Frame  0 : Logical Channel UL-DCCH, Received LTE_RRCConnectionReconfigurationComplete, reconfiguring DRB 1/LCID 3
7320446.583074 [MAC] I UE 0 RNTI bd8c adding LC 3 idx 2 to scheduling control (total 3)
7320446.583077 [MAC] I Added physicalConfigDedicated 0x7fd18c46be50 for 0.0
7320446.583095 [S1AP] I initial_ctxt_resp_p: e_rab ID 5, enb_addr 192.168.61.20, SIZE 4 
7320446.583150 [SCTP] I Successfully sent 40 bytes on stream 1 for assoc_id 4120
7320446.595495 [SCTP] I Successfully sent 61 bytes on stream 1 for assoc_id 4120
7320446.774197 [SCTP] I Found data for descriptor 103
7320446.774227 [SCTP] I [4120][103] Msg of length 53 received, on stream 1, PPID 18
7320446.774295 [RRC] I [eNB 0] Received S1AP_DOWNLINK_NAS: ue_initial_id 0, eNB_ue_s1ap_id 4527568
7320446.774316 [RRC] I sent RRC_DCCH_DATA_REQ to TASK_PDCP_ENB
...
```

On the MME: be patient, the statistics display update is slow!

```bash
$ docker exec rfsim4g-magma-mme /bin/bash -c "cat /var/log/mme.log"
000413 Fri Feb 11 09:52:52 2022 7EFFE8BE6700 INFO  GTPv2- lib/gtpv2-c/nwgtpv2c-0.11/src/Nw:2376    Stopped active timer 0x2 for info 0x0x60700001a9b0!
000414 Fri Feb 11 09:52:52 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000415 Fri Feb 11 09:52:52 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_main.c    :0083    MME APP ZMQ latency: 172.
000416 Fri Feb 11 09:52:52 2022 7EFFEA3E9700 INFO  MME-AP tasks/mme_app/mme_app_main.c    :0137    Received S11 MODIFY BEARER RESPONSE from SPGW
000417 Fri Feb 11 09:52:52 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_main.c    :0150    S11 MODIFY BEARER RESPONSE local S11 teid = 1
000418 Fri Feb 11 09:52:52 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000419 Fri Feb 11 09:53:28 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000420 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0049    ======================================= STATISTICS ============================================

000421 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0050                   |   Current Status|
000422 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0053    Attached UEs   |          0      |
000423 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0056    Connected UEs  |          0      |
000424 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0059    Connected eNBs |          1      |
000425 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0062    Default Bearers|          0      |
000426 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0065    S1-U Bearers   |          0      |

000427 Fri Feb 11 09:53:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0070    ======================================= STATISTICS ============================================

000428 Fri Feb 11 09:54:28 2022 7EFFEA3E9700 DEBUG MME-AP tasks/mme_app/mme_app_state_mana:0097    Inside get_state with read_from_db 0
000429 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0049    ======================================= STATISTICS ============================================

000430 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0050                   |   Current Status|
000431 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0053    Attached UEs   |          1      |
000432 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0056    Connected UEs  |          1      |
000433 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0059    Connected eNBs |          1      |
000434 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0062    Default Bearers|          1      |
000435 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0065    S1-U Bearers   |          1      |

000436 Fri Feb 11 09:54:28 2022 7EFFEC3ED700 DEBUG SERVIC tasks/service303/service303_mme_:0070    ======================================= STATISTICS ============================================
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
Stopping rfsim4g-magma-mme      ... done
Stopping rfsim4g-oai-hss        ... done
Stopping rfsim4g-redis          ... done
Stopping rfsim4g-trf-gen        ... done
Stopping rfsim4g-cassandra      ... done
Removing rfsim4g-oai-lte-ue0    ... done
Removing rfsim4g-oai-enb        ... done
Removing rfsim4g-oai-spgwu-tiny ... done
Removing rfsim4g-oai-spgwc      ... done
Removing rfsim4g-magma-mme      ... done
Removing rfsim4g-oai-hss        ... done
Removing rfsim4g-redis          ... done
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

All UE parameter values will be taken from the mounted config file, so modify
the corresponding UE file to reflect the above changes

```yaml
  oai_ue0:
        [...]
        volumes:
            - ../../conf_files/lteue.usim-ci.conf:/opt/oai-lte-ue/etc/ue_usim.conf
```

and in the UE file, you should have
```
PLMN: {
    PLMN0: {
           FULLNAME="OpenAirInterface";
           SHORTNAME="OAICN";
           MNC="96";
           MCC="208";
    };
};
[...]
    SIM: {
        MSIN="0100000001";
        USIM_API_K="fec86ba6eb707ed08905757b1bb44b8f";
        OPC="c42449363bbad02b66d16bc975d77cc1";
        MSISDN="001011234561010";
    };
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

in MME config, in the docker-compose there is not much besides REALM fields:

```yaml
            TZ: Europe/Paris
            REALM: openairinterface.org
            PREFIX: /openair-mme/etc
            HSS_HOSTNAME: hss
            HSS_FQDN: hss.openairinterface.org
            HSS_REALM: openairinterface.org
            MME_FQDN: mme.openairinterface.org
            FEATURES: mme_oai
```

Everything is hard-coded in the `mme.conf` that is mounted as a volume: look for instances of

```
MCC="208" ; MNC="96";  TAC = "1";
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

In eNB, all the parameter values will be taken from the mounted config file.

```yaml
oai_enb0:
        [...]
        volumes:
            - ../../conf_files/enb.band7.25prb.rfsim.conf:/opt/oai-enb/etc/enb.conf
```

with the eNB config reading:

```libconfig
    plmn_list = ( { mcc = 208; mnc = 96; mnc_length = 2; } );
```

The values SHALL match, and `TAC` shall match `TAC_0` from MME.

## 5.3. Access to Internet ##

In my traffic test, I was able to ping outside of my local network.

in SPGW-C config:

```yaml
            DEFAULT_DNS_IPV4_ADDRESS: 172.21.3.100
            DEFAULT_DNS_SEC_IPV4_ADDRESS: 8.8.4.4
            PUSH_PROTOCOL_OPTION: 'true'
```

in SPGW-U config:

```yaml
            NETWORK_UE_NAT_OPTION: 'yes'
```

Please put your own DNS server IP adress.

And you may have to play with `PUSH_PROTOCOL_OPTION` and `NETWORK_UE_NAT_OPTION` depending on your network.

