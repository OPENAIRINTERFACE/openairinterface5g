<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI 5G SA tutorial</font></b>
    </td>
  </tr>
</table>

**TABLE OF CONTENTS**

1. [SA setup with COTS UE](#1--sa-setup-with-cots-ue)
   1.  [gNB build and configuration](#11--gnb-build-and-configuration)
   2.  [OAI 5G Core Network installation and configuration](#12--oai-5g-core-network-installation-and-configuration)
   3.  [Execution of SA scenario](#13--execution-of-sa-scenario)
2. [SA Setup with OAI NR UE Softmodem](#2-sa-setup-with-oai-nr-ue-softmodem)
   1.  [Build and configuration](#21-build-and-configuration)
   2.  [OAI 5G Core Network installation and configuration](#22--oai-5g-core-network-installation-and-configuration)
   3.  [Execution of SA scenario](#23-execution-of-sa-scenario)

In the following tutorial we describe how to deploy configure and test the two SA OAI setups:

 - SA setup with OAI gNB and COTS UE
 - SA setup with OAI gNB and OAI UE
 
The operating system and hardware requirements to support OAI 5G NR are described [here](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/5g-nr-development-and-setup). 

# 1.  SA setup with COTS UE
At the moment of writing this document interoperability with the following COTS UE devices is being tested:

 - [Quectel RM500Q-GL](https://www.quectel.com/product/5g-rm500q-gl/)
 - [Simcom SIMCOM8200EA](https://www.simcom.com/product/SIM8200EA_M2.html)
 - Huawei Mate 30 Pro

 End-to-end control plane signaling to achieve a 5G SA connection, UE registration and PDU session establishment with the CN, as well as some basic user-plane traffic tests have been validated so far using SIMCOM/Quectel modules and Huawei Mate 30 pro. In terms of interoperability with different 5G Core Networks, so far this setup has been tested with:
 

 - [OAI CN](https://openairinterface.org/oai-5g-core-network-project/)
 - Nokia SA Box
 - [Free CN](https://www.free5gc.org/)

 
## 1.1  gNB build and configuration
To get the code and build the gNB executable:

### Ubuntu 18.04
```bash
    git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
    git checkout develop
    cd openairinterface5g/
    source oaienv
    cd cmake_targets/
    ./build_oai -I -w USRP #For OAI first time installation only to install software dependencies
    ./build_oai --gNB -w USRP
```

### Ubuntu 20.04
```bash
    # Build UHD from source
    # https://files.ettus.com/manual/page_build_guide.html
    sudo apt-get install libboost-all-dev libusb-1.0-0-dev doxygen python3-docutils python3-mako python3-numpy python3-requests python3-ruamel.yaml python3-setuptools cmake build-essential
    
    git clone https://github.com/EttusResearch/uhd.git
    cd uhd/host
    mkdir build
    cd build
    cmake ../
    make -j 4
    make test # This step is optional
    sudo make install
    sudo ldconfig
    sudo uhd_images_downloader


    git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
    git checkout develop
    
    # Install dependencies in Ubuntu 20.04
    cd
    cd openairinterface5g/
    source oaienv
    cd cmake_targets/
    ./install_external_packages.ubuntu20
    
    # Build OAI gNB
    cd
    cd openairinterface5g/
    source oaienv
    cd cmake_targets/
    ./build_oai --gNB -w USRP
```

A reference configuration file for the gNB is provided  [here](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf).     


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
    ////////// AMF parameters:
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

**CAUTION:** the `192.168.70.132` AMF IF address is the OAI-CN5G `AMF` Container IP address. You certainly will need to do some networking manipulations for the `gNB` server to be able to see this AMF container.

Please read [CN5G tutorial for more details](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed/-/blob/master/README.md).

### **gNB configuration in CU/DU split mode**
For the configuration of the gNB in CU and DU blocks, the following sample configuration files are provided for the [CU](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/targets/PROJECTS/GENERIC-NR-5GC/CONF/cu_gnb.conf) and the [DU](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/targets/PROJECTS/GENERIC-NR-5GC/CONF/du_gnb.conf) entities respectively. These configuration files have to be updated with the IP addresses of the CU and the DU over the F1 interface. For example, in the following section from the DU configuration file, *local_n_address* corresponds to the DU address and *remote_n_address* corresponds to the CU address:

```bash
MACRLCs = (
  {
    num_cc           = 1;
    tr_s_preference  = "local_L1";
    tr_n_preference  = "f1";
    local_n_if_name = "lo";
    local_n_address = "127.0.0.3";
    remote_n_address = "127.0.0.4";
    local_n_portc   = 601;
    local_n_portd   = 2152;
    remote_n_portc  = 600;
    remote_n_portd  = 2152;

  }
);
```


At the point of writing this document the control-plane exchanges between the CU and the DU over *F1-C* interface, as well as some IP traffic tests over *F1-U* have been validated using the OAI gNB/nrUE in RFSIMULATOR mode. 

*These extensions are not yet fully integrated into develop branch, as they are under merge request. Until they get fully integrated, the CU/DU functionalities can be tested in [NR_F1C_F1U_extensions](https://gitlab.eurecom.fr/oai/openairinterface5g/-/tree/NR_F1C_F1U_extensions) branch.* 

## 1.2  OAI 5G Core Network installation and configuration
The instructions for the installation of OAI CN components (AMF, SMF, NRF, UPF) using `docker-compose` can be found [here](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed/-/blob/master/README.md).

## 1.3  Execution of SA scenario

After having configured the gNB, we can start the individual components in the following sequence:

 - Launch 5G Core Network
 - Launch gNB
 - Launch COTS UE (disable airplane mode)

The execution command to start the gNB (in monolithic mode) is the following:
```bash
cd cmake_targets/ran_build/build
sudo ./nr-softmodem -E --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf
```

# 2. SA Setup with OAI NR UE Softmodem
The SA setup with OAI UE has been validated with **RFSIMULATOR**. Both control plane and user plane for the successful UE registration and PDU Session establishment has been verified with OAI and Nokia SA Box CNs.

In the following, we provide the instructions on how to build, configure and execute this SA setup.

### NAS configuration for the OAI UE
The NAS configuration parameters of the OAI UE can be set as input parameters, configuration file or can be hardcoded.  More specifically:
- SUCI (*Subscription Concealed Identifier*)
- USIM_API_K and OPc keys
- NSSAI (*Network Slice Assistance Information*)
- DNN (*Data Network Name*)

Below is a sample configuration file that can be parsed through the execution command ([section 2.3](#23-execution-of-sa-scenario)).

```bash
uicc0 = {
imsi = "208990000007487";
key = "fec86ba6eb707ed08905757b1bb44b8f";
opc= "C42449363BBAD02B66D16BC975D77CC1";
dnn= "oai";
nssai_sst=1;
nssai_sd=1;
}
```

Alternatively, the values can be hardcoded/edited in source file ***openair3/UICC/usim_interface.c*** through the following lines:
```bash
#define UICC_PARAMS_DESC {\
    {"imsi",             "USIM IMSI\n",          0,         strptr:&(uicc->imsiStr),              defstrval:"2089900007487",           TYPE_STRING,    0 },\
    {"nmc_size"          "number of digits in NMC", 0,      iptr:&(uicc->nmc_size),               defintval:2,         TYPE_INT,       0 },\
    {"key",              "USIM Ki\n",            0,         strptr:&(uicc->keyStr),               defstrval:"fec86ba6eb707ed08905757b1bb44b8f", TYPE_STRING,    0 },\
    {"opc",              "USIM OPc\n",           0,         strptr:&(uicc->opcStr),               defstrval:"c42449363bbad02b66d16bc975d77cc1", TYPE_STRING,    0 },\
    {"amf",              "USIM amf\n",           0,         strptr:&(uicc->amfStr),               defstrval:"8000",    TYPE_STRING,    0 },\
    {"sqn",              "USIM sqn\n",           0,         strptr:&(uicc->sqnStr),               defstrval:"000000",  TYPE_STRING,    0 },\
    {"dnn",              "UE dnn (apn)\n",       0,         strptr:&(uicc->dnnStr),               defstrval:"oai",     TYPE_STRING,    0 },\
    {"nssai_sst",        "UE nssai\n",           0,         iptr:&(uicc->nssai_sst),              defintval:1,    TYPE_INT,    0 }, \
    {"nssai_sd",         "UE nssai\n",           0,         iptr:&(uicc->nssai_sd),               defintval:1,    TYPE_INT,    0 }, \
  };
```

For interoperability with OAI or other CNs, it should be ensured that the configuration of the aforementioned parameters match the configuration of the corresponding subscribed user at the core network.


## 2.1 Build and configuration
To build the gNB and OAI UE executables:  

```bash
    cd cmake_targets
    # Note: For OAI first time installation please install software dependencies as described in 1.1.
    ./build_oai --gNB --nrUE -w SIMU
```
The gNB configuration can be performed according to what is described in [section 1.1](#11--gnb-build-and-configuration), using the same reference configuration file as with the RF scenario.

## 2.2  OAI 5G Core Network installation and configuration
The instructions for the installation of OAI CN components (AMF, SMF, NRF, UPF) using `docker-compose` can be found [here](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed/-/blob/master/README.md).

In addition, if you do not want to build anything, please have a look at [this tutorial](https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/ci-scripts/yaml_files/5g_rfsimulator/README.md).

## 2.3 Execution of SA scenario

The order of starting the different components should be the same as the one described in [section 1.3](#13--execution-of-sa-scenario).

the gNB can be launched in 2 modes:

- To launch the gNB in `monolithic` mode:
 ```bash
 sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa \
     -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf
 ```
- To launch the gNB in `CU/DU split` mode:

    1. Launch the CU component:
    ```bash
    sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa \
        -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/cu_gnb.conf
    ```
    2. Launch the DU component:
    ```bash
    sudo RFSIMULATOR=server ./nr-softmodem --rfsim --sa \
        -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/du_gnb.conf
    ```

- To launch the OAI UE (valid in `monolithic` gNB and `CU/DU split` gNB):
 ```bash
sudo RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 \
    --rfsim --sa --nokrnmod -O <PATH_TO_UE_CONF_FILE>
```

The IP address at the execution command of the OAI UE corresponds to the target IP of the gNB host that the RFSIMULATOR at the UE will connect to. In the above example, we assume that the gNB and UE are running on the same host so the specified address (127.0.0.1) is the one of the loopback interface.  
