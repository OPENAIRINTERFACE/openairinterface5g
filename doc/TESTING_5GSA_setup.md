
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
At the moment of writing this document most of the code to support the SA setup is not merged into develop branch yet, but it is accessible through the following branches:

 - NR_SA_F1AP_5GRECORDS
 - develop-NR_SA_F1AP_5GRECORDS (up-to-date with latest develop branch)

To build the gNB executable:
```bash
    cd cmake_targets
    ./build_oai -I -w USRP #For first time installation only to install software dependencies
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
The instructions for the installation of OAI CN components (AMF, SMF, NRF, UPF) using docker compose can be found [here](https://gitlab.eurecom.fr/oai/cn5g). Below are some complementary instructions which can be useful for the deployment...


 ## 1.3  Execution of SA scenario

After having configured the gNB, we can start the individual components in the following sequence:

 - Launch Core Network
 - Launch gNB
 - Launch COTS UE (disable airplane mode)

The execution command to start the gNB is the following:
```bash
cd cmake_targets/ran_build/cuild
sudo ./nr-softmodem -E --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf
```	

# 2. SA Setup with OAI UE 
For the setup with the OAI UE
