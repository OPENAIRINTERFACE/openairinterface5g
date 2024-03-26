# OpenAirInterface License #

 *  [OAI License Model](http://www.openairinterface.org/?page_id=101)
 *  [OAI License v1.1 on our website](http://www.openairinterface.org/?page_id=698)

It is distributed under **OAI Public License V1.1**.

The license information is distributed under [LICENSE](LICENSE) file in the same directory.

Please see [NOTICE](NOTICE.md) file for third party software that is included in the sources.

# Overview

This tutorial describes the steps of deployment 5G OAI RAN, with integrated E2 agent and a nearRT-RIC using O-RAN compliant FlexRIC.

[[_TOC_]]

# 1. Installation

## 1.1 Install prerequisites

- A *recent* CMake (at least v3.15). 

  On Ubuntu, you might want to use [this PPA](https://apt.kitware.com/) to install an up-to-date version.

- SWIG (at least  v.4.1). 

  We use SWIG as an interface generator to enable the multi-language feature (i.e., C/C++ and Python) for the xApps. Please, check your SWIG version (i.e, `swig
  -version`) and install it from scratch if necessary as described here: https://swig.org/svn.html or via the code below: 
  
  ```bash
  git clone https://github.com/swig/swig.git
  cd swig
  git checkout release-4.1 
  ./autogen.sh
  ./configure --prefix=/usr/
  make -j8
  make install
  ```

- Flatbuffer encoding(optional). 
  
  We also provide a flatbuffers encoding/decoding scheme as alternative to ASN.1. In case that you want to use it  follow the
  instructions at https://github.com/dvidelabs/flatcc and provide the path for the lib and include when selecting it at `ccmake ..` from the build directory 

## 1.2 Download the required dependencies. 

Below an example of how to install it in ubuntu
```bash
sudo apt install libsctp-dev python3.8 cmake-curses-gui libpcre2-dev python-dev
```

# 2. Deployment

## 2.1 OAI RAN

### 2.1.1 Clone the OAI repository
```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g oai
cd oai/
```

### 2.1.2 Build OAI with E2 Agent

- By default, OAI will build the E2 Agent with E2AP v2 and KPM v2. If you want a different version, edit the variable E2AP\_VERSION and KPM\_VERSION at OAI's CMakeLists.txt file.

Currently available versions:
|            |KPM v2.03|KPM v3.00|
|:-----------|:--------|:--------|
| E2AP v1.01 | Y       | Y       |
| E2AP v2.03 | Y       | Y       |
| E2AP v3.01 | Y       | Y       |

Please note that KPM SM v2.01 is supported only in FlexRIC, but not in OAI.

```bash
cd cmake_targets/
./build_oai -I -w SIMU --gNB --nrUE --build-e2 --ninja
```

 * `-I` option is to install pre-requisites, you only need it the first time you build the softmodem or when some oai dependencies have changed.
 * `-w` option is to select the radio head support you want to include in your build. Radio head support is provided via a shared library, which is called the "oai device" The build script creates a soft link from `liboai_device.so` to the true device which will be used at run-time (here the USRP one, liboai_usrpdevif.so). The RF simulatorRF simulator is implemented as a specific device replacing RF hardware, it can be specifically built using `-w SIMU` option, but is also built during any softmodem build.
 * `--gNB` is to build the `nr-softmodem` and `nr-cuup` executables and all required shared libraries
 * `--nrUE` is to build the `nr-uesoftmodem` executable and all required shared libraries
 * `--ninja` is to use the ninja build tool, which speeds up compilation
 * `--build-e2` option is to use the E2 agent, integrated within RAN.

If the openair2/E2AP/flexric folder is empty, try manually the following commands:
```bash
git submodule init
git submodule update
```

## 2.2 FlexRIC

By default, FlexRIC will build the nearRT-RIC with E2AP v2 and KPM v2. If you want a different version, edit the variable E2AP\_VERSION and KPM\_VERSION at FlexRIC's CMakeLists.txt file. Note that OAI's and FlexRIC's E2AP\_VERSION and KPM\_VERSION need to match due to O-RAN incompatibilities among versions.


### 2.2.1 Clone the FlexRIC repository
```bash
git clone https://gitlab.eurecom.fr/mosaic5g/flexric flexric
cd flexric/
git checkout 34d6810f9ab435ac3a4023d5e1917669e24341da
```

### 2.2.2 Build FlexRIC
```bash
mkdir build && cd build && cmake .. && make -j8
```

### 2.2.3 Installation of Service Models (SMs)
```bash
sudo make install
```

By default the service model libraries will be installed in the path `/usr/local/lib/flexric` while the configuration file in `/usr/local/etc/flexric`.

# 3. Service Models available in OAI RAN

## 3.1 O-RAN

We assume that user is familiar with O-RAN WG3 specifications that can be found at https://orandownloadsweb.azurewebsites.net/specifications.

### 3.1.1 E2SM-KPM

As mentioned in section [2.1.2 Build OAI with E2 Agent](#212-build-oai-with-e2-agent), we support KPM v2.03/v3.00. Uses ASN.1 encoding.

Per O-RAN specifications, 5G measurements supported by KPM are specified in 3GPP TS 28.552.

From 3GPP TS 28.552, we support the following list:
  * "DRB.PdcpSduVolumeDL"
  * "DRB.PdcpSduVolumeUL"
  * "DRB.RlcSduDelayDl"
  * "DRB.UEThpDl"
  * "DRB.UEThpUl"
  * "RRU.PrbTotDl"
  * "RRU.PrbTotUl"

From O-RAN.WG3.E2SM-KPM-version specification, we implemented:
  * REPORT Service Style 4 ("Common condition-based, UE-level" - section 7.4.5) - fetch above measurements per each UE that matches common criteria (e.g. S-NSSAI).

### 3.1.2 E2SM-RC

We support RC v1.03. Uses ASN.1 encoding.

From ORAN.WG3.E2SM-RC-v01.03 specification, we implemented:
  * REPORT Service Style 4 ("UE Information" - section 7.4.5) - aperiodic subscription for "UE RRC State Change"
  * CONTROL Service Style 1 ("Radio Bearer Control" - section 7.6.2) - "QoS flow mapping configuration" (e.g creating a new DRB)

## 3.2 Custom Service Models

In addition, we support custom Service Models, such are MAC, RLC, PDCP, and GTP. Use plain encoding.

If you are interested in TC and SLICE SMs, please follow the instructions at https://gitlab.eurecom.fr/mosaic5g/flexric.

# 4. Start the process

At this point, we assume the 5G Core Network is already running in the background. For more information, please follow the tutorial at https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/doc/NR_SA_Tutorial_OAI_CN5G.md.

In order to configure E2 agent, please, add the following block in OAI's configuration file:
```bash
e2_agent = {
  near_ric_ip_addr = "127.0.0.1";
  sm_dir = "/usr/local/lib/flexric/"
}
```
* start E2 Node agents

  As per O-RAN.WG3.E2SM-v02.00 specifications, UE ID (section 6.2.2.6) representation in OAI is:
  |                       | gNB-mono        | CU              | CU-CP           | CU-UP                 | DU                 |
  |:----------------------|:----------------|:----------------|:----------------|:----------------------|:-------------------|
  | CHOICE UE ID case     | GNB_UE_ID_E2SM  | GNB_UE_ID_E2SM  | GNB_UE_ID_E2SM  | GNB_CU_UP_UE_ID_E2SM  | GNB_DU_UE_ID_E2SM  |
  | AMF UE NGAP ID        | amf_ue_ngap_id  | amf_ue_ngap_id  | amf_ue_ngap_id  |                       |                    |
  | GUAMI                 | guami           | guami           | guami           |                       |                    |
  | gNB-CU UE F1AP ID     |                 | rrc_ue_id       |                 |                       | rrc_ue_id          |
  | gNB-CU-CP UE E1AP ID  |                 |                 | rrc_ue_id       | rrc_ue_id             |                    |
  | RAN UE ID             | rrc_ue_id       | rrc_ue_id       | rrc_ue_id       | rrc_ue_id             | rrc_ue_id          |

  * start the gNB-mono
    ```bash
    cd oai/cmake_targets/ran_build/build
    sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --rfsim --sa -E
    ```

  * if CU/DU split is used, start the gNB as follows
    ```bash
    sudo ./nr-softmodem -O <path_to_du_conf_file> --rfsim --sa -E
    sudo ./nr-softmodem -O <path_to_cu_conf_file> --sa
    ```

  * if CU-CP/CU-UP/DU split is used, start the gNB as follows
    ```bash
    sudo ./nr-softmodem -O <path_to_du_conf_file> --rfsim --sa -E
    sudo ./nr-softmodem -O <path_to_cu_cp_conf_file> --sa
    sudo ./nr-cuup -O <path_to_cu_up_conf_file> --sa
    ```

* start the nrUE
  ```bash
  cd oai/cmake_targets/ran_build/build
  sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa --uicc0.imsi 001010000000001 --rfsimulator.serveraddr 127.0.0.1
  ```

* start the nearRT-RIC
  ```bash
  cd flexric
  ./build/examples/ric/nearRT-RIC
  ```

* Start different xApps

  * start the KPM monitor xApp - measurements stated in [3.1.1 E2SM-KPM](#311-e2sm-kpm) for each UE that matches S-NSSAI common criteria
    ```bash
    cd flexric
    ./build/examples/xApp/c/monitor/xapp_kpm_moni
    ```
    Note: we assume that each UE has only 1 DRB; CU-UP does not store the slices, therefore "coarse filtering" is used

  * start the RC monitor xApp - aperiodic subscription for "UE RRC State Change"
    ```bash
    cd flexric
    ./build/examples/xApp/c/monitor/xapp_rc_moni
    ```

  * start the RC control xApp - RAN control function "QoS flow mapping configuration" (e.g. creating a new DRB)
    ```bash
    cd flexric
    ./build/examples/xApp/c/kpm_rc/xapp_kpm_rc
    ```

  * start the (MAC + RLC + PDCP + GTP) monitor xApp
    ```bash
    cd flexric
    ./build/examples/xApp/c/monitor/xapp_gtp_mac_rlc_pdcp_moni
    ```
The latency that you observe in your monitor xApp is the latency from the E2 Agent to the nearRT-RIC and xApp. 
Therefore, FlexRIC is well suited for use cases with ultra low-latency requirements.
Additionally, all the data received in the `xapp_gtp_mac_rlc_pdcp_moni` xApp is also written to /tmp/xapp_db in case that offline data processing is wanted (e.g., Machine Learning/Artificial Intelligence applications). You can browse the data using e.g., sqlitebrowser.
Please note:
* KPM SM database is not been updated, therefore commented in `flexric/src/xApp/db/sqlite3/sqlite3_wrapper.c:1152`
* RC SM database is not yet implemented.

# Optional - Multiple UEs

If you are interested in having multiple UEs in rfsim mode, please, follow the instructions at https://gitlab.eurecom.fr/oaiworkshop/summerworkshop2023/-/tree/main/ran#multiple-ues.
