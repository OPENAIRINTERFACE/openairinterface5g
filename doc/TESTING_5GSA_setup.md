
# OAI 5G SA tutorial [Under construction]

In the following tutorial we describe how to deploy configure and test the two SA OAI setups:

 - SA setup with OAI gNB and COTS UE
 - SA setup with OAI gNB and OAI UE
 
The operating system and hardware requirements to support OAI 5G NR are described [here](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/5g-nr-development-and-setup). 

# 1.  SA setup with COTS UE
At the moment of writing this document interoperability with the following COTS UE devices is being tested:

 - [Quectel RM500Q-GL](https://www.quectel.com/product/5g-rm500q-gl/)
 - [Simcom SIMCOM8200EA](https://www.simcom.com/product/SIM8200G.html)
 - Huawei Mate 30 Pro

 End-to-end control plane signaling to achieve a 5G SA connection, UE registration and PDU session establishment with the CN, as well as some basic user-plane traffic tests have been validated so far using the Quectel module and Huawei Mate 30 pro and partially validated with SIMCOM module. In terms of interoperability with different 5G Core Networks, so far this setup has been tested with:
 

 - [OAI CN](https://openairinterface.org/oai-5g-core-network-project/)
 - Nokia SA Box
 - [Free CN](https://www.free5gc.org/)

 
 ## 1.1  gNB build and configuration
At the moment of writing this document, most of the code to support the SA setup is not merged into develop branch yet, but it is accessible through the following branches:

 - NR_SA_F1AP_5GRECORDS
 - develop-NR_SA_F1AP_5GRECORDS (up-to-date with latest develop branch)

To build the gNB executable:
```bash
    cd cmake_targets
    ./build_oai -I -w USRP #For OAI first time installation only to install software dependencies
    ./build_oai --gNB -w USRP
```

A reference configuration file for the gNB is provided  [here](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop-NR_SA_F1AP_5GRECORDS/targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf).     


In the following, we highlight the fields of the file that have to be configured according to the configuration and interfaces of the Core Network. First, the PLMN section has to be filled with the proper values that match the configuration of the AMF and the UE USIM.
```bash
    // Tracking area code, 0x0000 and 0xfffe are reserved values
    tracking_area_code  =  1;
    plmn_list = ({
			 mcc = 208;
			 mnc = 99;
			 mnc_length = 2;
			 snssaiList = (
			 {
				sst = 1;
				sd  = 0x1; // 0 false, else true
			 },
			 {
				 sst = 1;
				 sd  = 0x112233; // 0 false, else true
			 }
			);
		});
```		
Then, the source and destination IP interfaces for the communication with
the Core Network also need to be set as shown below.

```bash
	////////// MME parameters:
	 amf_ip_address      = ( { ipv4       = "192.168.70.132";
			           ipv6       = "192:168:30::17";
				   active     = "yes";
				   preference = "ipv4";
				 }
			       );
	 NETWORK_INTERFACES :
	 {
		 GNB_INTERFACE_NAME_FOR_NG_AMF            = "demo-oai";
		 GNB_IPV4_ADDRESS_FOR_NG_AMF              = "192.168.70.129/24";
		 GNB_INTERFACE_NAME_FOR_NGU               = "demo-oai";
		 GNB_IPV4_ADDRESS_FOR_NGU                 = "192.168.70.129/24";
		 GNB_PORT_FOR_S1U                         = 2152; # Spec 2152
	 };
```	 
In the first part (*amf_ip_address*) we specify the IP of the AMF and in the second part (*NETWORK_INTERFACES*) we specify the gNB local interface with AMF (N2 interface) and the UPF (N3 interface).

### **gNB configuration in CU/DU split mode**
For the configuration of the gNB in CU and DU blocks the following sample configuration files are provided for the CU and DU entities respectively. 
......
At the point of writing this document the control-plane exchanges between the CU and the DU over *F1-C* interface have been validated. The integration of *F1-U* over gtp-u for the support of data plane traffic is ongoing.

## 1.2  OAI 5G Core Network installation and configuration
The instructions for the installation of OAI CN components (AMF, SMF, NRF, UPF) using docker compose can be found [here](https://gitlab.eurecom.fr/oai/cn5g). Below are some complementary instructions which can be useful for the deployment.

 ## 1.3  Execution of SA scenario

After having configured the gNB, we can start the individual components in the following sequence:

 - Launch Core Network
 - Launch gNB
 - Launch COTS UE (disable airplane mode)

The execution command to start the gNB (in monolithic mode) is the following:
```bash
cd cmake_targets/ran_build/build
sudo ./nr-softmodem -E --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf
```	

# 2. SA Setup with OAI UE 
The SA setup with OAI UE has been validated with RFSIMULATOR for the moment. The control plane for the successful UE registration and PDU Session establishment has been verified with OAI and Nokia SA Box CNs. User-plane traffic validation after the establishment of the 5G connection is still pending for this setup. 

In the following, we provide the instructions on how to build, configure and execute this SA setup. 

## 2.1 Build and configuration
To build the gNB and OAI UE executables:  

```bash
    cd cmake_targets
    ./build_oai -I #For OAI first time installation only to install software dependencies
    ./build_oai --gNB --nrUE -w SIMU
```
The gNB configuration can be performed according to what is described in section 1.1, using the same reference configuration file as with the RF scenario.

### NAS configuration for the OAI UE
At the moment, the NAS configuration parameters of the OAI UE are hardcoded in ***openair3/NAS/NR_UE/nr_nas_msg_sim.c***.  More specifically:

 - The SUCI (*Subscription Concealed Identifier*) corresponding to default IMSI 2089900007487 is hardcoded in functions *generateRegistrationRequest()* and *generateIdentityResponse()* through the following lines:
```bash
    mm_msg->registration_request.fgsmobileidentity.suci.typeofidentity = FGS_MOBILE_IDENTITY_SUCI;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit1 = 9;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit2 = 9;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit3 = 0xf;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit1 = 2;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit2 = 0;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit3 = 8;
    mm_msg->registration_request.fgsmobileidentity.suci.schemeoutput = 0x4778;
```
 - USIM_API_K and OPc keys are hardcoded at the beginning of the file:
```bash
// USIM_API_K: fe c8 6b a6 eb 70 7e d0 89 05 75 7b 1b b4 4b 8f 
uint8_t k[16] = {0xfe, 0xc8, 0x6b, 0xa6, 0xeb, 0x70, 0x7e, 0xd0, 0x89, 0x05, 0x75, 0x7b, 0x1b, 0xb4, 0x4b, 0x8f};
// OPC: c4 24 49 36 3b ba d0 2b 66 d1 6b c9 75 d7 7c c1
const uint8_t opc[16] = {0xc4, 0x24, 0x49, 0x36, 0x3b, 0xba, 0xd0, 0x2b, 0x66, 0xd1, 0x6b, 0xc9, 0x75, 0xd7, 0x7c, 0xc1};
```
-  The NSSAI (*Network Slice Assistance Information*) and DNN (*Data Network Name*) are hardcoded in function *generatePduSessionEstablishRequest()*
```bash
  uint8_t             nssai[]={1,0,0,1}; //Corresponding to SST:1, SD:1
  uint8_t             dnn[4]={0x4,0x6f,0x61,0x69}; //Corresponding to dnn:"oai"
```
For interoperability with OAI or other CNs, it should be ensured that the configuration of the aforementioned parameters match the configuration of the corresponding subscribed user at the core network.
Hardcoding of the USIM information will soon be substituted with parsing those parameters from a configuration file. 

## 2.2 Execution of SA scenario

The order of starting the different components should be the same as the one described in section 1.3. 

 - To launch the gNB:
 ```bash
 sudo RFSIMULATOR=server ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --rfsim --sa
 ```
- To launch the OAI UE:
 ```bash
sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa --nokrnmod
```
The IP address at the execution command of the OAI UE corresponds to the target IP of the gNB host that the RFSIMULATOR at the UE will connect to. In the above example, we assume that the gNB and UE are running on the same host so the specified address (127.0.0.1) is the one of the loopback interface.  
