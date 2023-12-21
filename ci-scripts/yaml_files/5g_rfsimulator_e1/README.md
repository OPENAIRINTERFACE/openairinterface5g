# CI test for 5G F1+E1 splits with RFsimulator

## General

This docker-compose file deploys a core, RAN, and 3 UEs. Specifically:

- 5GC mini deployment,
- 1 CU-CP,
- 3 CU-UPs, one for slices SST=1,2,3,
- 3 DUs, 
- 3 UEs connecting to each DU, each requesting slice SST=1,2,3.

The CU-CP associates each UE X connecting through DU X to CU-UP X, X=1,2,3.  To
this end, the docker-compose file deliberately employs the three `ran_netX`
networks to route the user-plane traffic of each DU X/CU-UP X pair, through the
`ran_netX` network in order to effectively test that the correct CU-UP is
associated. Basically, the traffic test will only succeed if the correct pair
of DU/CU-UP has been established; if not, the DU/CU-UP cannot communicate, as
the traffic cannot be routed.

For all other (CP, RFsim) traffic, all entities use the `public_net` network. 

## How to run

You should be able to deploy the [basic 5G RFsim deployment](../5g_rfsimulator/README.md).

First, start the MySQL server and 5GC:
```bash
$ docker-compose up -d mysql oai-nrf oai-amf oai-smf oai-spgwu oai-ext-dn
$ docker-compose ps -a
```

Wait till everything is healthy.

Next, start the RAN:
```bash
$ docker-compose up -d oai-cucp oai-cuup{,2,3} oai-du{,2,3}
```

You can verify that the DUs and CU-UPs connected successfully:
```bash
$ docker logs rfsim5g-oai-cucp
[...]
18535.139811 [RRC] I Accepting new CU-UP ID 3585 name gNB-OAI (assoc_id 257)
18535.425744 [RRC] I Accepting new CU-UP ID 3584 name gNB-OAI (assoc_id 260)
18535.425757 [RRC] I Accepting new CU-UP ID 3586 name gNB-OAI (assoc_id 261)
18535.669733 [NR_RRC] I Received F1 Setup Request from gNB_DU 3585 (du-rfsim) on assoc_id 263
18535.669814 [RRC] I Accepting DU 3585 (du-rfsim), sending F1 Setup Response
18536.066417 [NR_RRC] I Received F1 Setup Request from gNB_DU 3586 (du-rfsim) on assoc_id 265
18536.066476 [RRC] I Accepting DU 3586 (du-rfsim), sending F1 Setup Response
18536.135581 [NR_RRC] I Received F1 Setup Request from gNB_DU 3584 (du-rfsim) on assoc_id 267
18536.135650 [RRC] I Accepting DU 3584 (du-rfsim), sending F1 Setup Response
```

You should see that the CU-UP initialized two GTP instances (one for NG-U, the
other for F1-U):
```
$ docker logs -f rfsim5g-oai-cuup
[...]
18535.425535 [GTPU] I Initializing UDP for local address 192.168.72.161 with port 2152
18535.425571 [GTPU] I Created gtpu instance id: 96
18535.425577 [GTPU] I Configuring GTPu address : 192.168.71.161, port : 2152
18535.425581 [GTPU] I Initializing UDP for local address 192.168.71.161 with port 2152
18535.425604 [GTPU] I Created gtpu instance id: 97
```

You should see the typical periodical output at the DUs:
```
$ docker logs rfsim5g-oai-du
[...]
18626.446953 [NR_MAC] I Frame.Slot 128.0

18629.151076 [NR_MAC] I Frame.Slot 256.0
```

Next, connect the UEs. They are configured to connect to each DU by setting the
RFsimulator server address to the `public_net` IP address of each DU. For each,
you should see that they get an IP address
```bash
$ docker compose up -d oai-nr-ue{,2,3}
$ docker logs oai-nr-ue
[...]
18758.176149 [NR_RRC] I rrcReconfigurationComplete Encoded 10 bits (2 bytes)
18758.176153 [NR_RRC] I  Logical Channel UL-DCCH (SRB1), Generating RRCReconfigurationComplete (bytes 2)
18758.176154 [NAS] I [UE 0] Received NAS_CONN_ESTABLI_CNF: errCode 1, length 87
18758.176455 [OIP] I Interface oaitun_ue1 successfully configured, ip address 12.1.1.3, mask 255.255.255.0 broadcast address 12.1.1.255
```

Alternatively, check that they all received an IP address (the associated IP
addresses might be different):
```bash
$ docker exec -it rfsim5g-oai-nr-ue3 ip a show oaitun_ue1
[...]
    inet 12.1.1.2/24 brd 12.1.1.255 scope global oaitun_ue1
[...]
$ docker exec -it rfsim5g-oai-nr-ue2 ip a show oaitun_ue1
[...]
    inet 12.1.1.4/24 brd 12.1.1.255 scope global oaitun_ue1
[...]
$ docker exec -it rfsim5g-oai-nr-ue ip a show oaitun_ue1
[...]
    inet 12.1.1.3/24 brd 12.1.1.255 scope global oaitun_ue1
[...]
```

Also, note that each DU sees only one UE! At the CU-CP, you should see that
each DU has been associated to a different CU-UP, based on the NSSAI (`exact
NSSAI match`):
```bash
$ docker logs rfsim5g-oai-cucp | grep CU-U
[...]
18757.531423 [RRC] I selecting CU-UP ID 3586 based on exact NSSAI match (3:0xffffff)
18757.531434 [RRC] I UE 1 associating to CU-UP assoc_id 261 out of 3 CU-UPs
18758.171502 [RRC] I selecting CU-UP ID 3584 based on exact NSSAI match (1:0xffffff)
18758.171510 [RRC] I UE 2 associating to CU-UP assoc_id 260 out of 3 CU-UPs
18758.772320 [RRC] I selecting CU-UP ID 3585 based on exact NSSAI match (2:0xffffff)
18758.772327 [RRC] I UE 3 associating to CU-UP assoc_id 257 out of 3 CU-UPs
```

Also, each UE should be able to ping the core network. For instance, with UE 1:
```bash
$ docker exec -it rfsim5g-oai-nr-ue ping -c1 12.1.1.1
PING 12.1.1.1 (12.1.1.1) 56(84) bytes of data.
64 bytes from 12.1.1.1: icmp_seq=1 ttl=64 time=15.2 ms

--- 12.1.1.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 15.155/15.155/15.155/0.000 ms
```

Finally, undeploy the UEs (to give them time to do deregistration), and then
the rest of the network:
```bash
$ docker compose stop oai-nr-ue{,2,3}
$ docker compose down
```
