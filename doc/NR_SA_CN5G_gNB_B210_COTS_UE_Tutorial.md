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

1. [Scenario](#1-scenario)
2. [OAI CN5G](#2-oai-cn5g)
    1. [OAI CN5G pre-requisites](#21-oai-cn5g-pre-requisites)
    2. [OAI CN5G Setup](#22-oai-cn5g-setup)
    3. [OAI CN5G Configuration files](#23-oai-cn5g-configuration-files)
    4. [SIM Card](#24-sim-card)
3. [OAI gNB](#3-oai-gnb)
    1. [OAI gNB pre-requisites](#31-oai-gnb-pre-requisites)
    2. [Build OAI gNB](#32-build-oai-gnb)
4. [Run OAI CN5G and OAI gNB with USRP B210](#4-run-oai-cn5g-and-oai-gnb-with-usrp-b210)
    1. [Run OAI CN5G](#41-run-oai-cn5g)
    2. [Run OAI gNB](#42-run-oai-gnb)
5. [Testing with QUECTEL RM500Q](#5-testing-with-quectel-rm500q)
    1. [Setup QUECTEL](#51-setup-quectel)
    2. [Ping test](#52-ping-test)
    3. [Downlink iPerf test](#53-downlink-iperf-test)


# 1. Scenario
In this tutorial we describe how to configure and run a 5G end-to-end setup with OAI CN5G, OAI gNB and COTS UE.

Minimum hardware requirements:
- Laptop/Desktop/Server for OAI CN5G and OAI gNB
    - Operating System: [Ubuntu 20.04.4 LTS](https://releases.ubuntu.com/20.04.4/ubuntu-20.04.4-desktop-amd64.iso)
    - CPU: 8 cores x86_64 @ 3.5 GHz
    - RAM: 32 GB
- Laptop for UE
    - Operating System: Microsoft Windows 10 x64
    - CPU: 4 cores x86_64
    - RAM: 8 GB
    - Windows driver for Quectel MUST be equal or higher than version **2.2.4**
- [USRP B210](https://www.ettus.com/all-products/ub210-kit/)
- Quectel RM500Q
    - Module, M.2 to USB adapter, antennas and SIM card
    - Firmware version of Quectel MUST be equal or higher than **RM500QGLABR11A06M4G**


# 2. OAI CN5G

## 2.1 OAI CN5G pre-requisites

```bash
sudo apt install -y git net-tools putty

sudo apt install -y apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu  $(lsb_release -cs)  stable"
sudo apt update
sudo apt install -y docker docker-ce

# Add your username to the docker group, otherwise you will have to run in sudo mode.
sudo usermod -a -G docker $(whoami)
reboot

# https://docs.docker.com/compose/install/
sudo curl -L "https://github.com/docker/compose/releases/download/1.29.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

docker network create --driver=bridge --subnet=192.168.70.128/26 -o "com.docker.network.bridge.name"="demo-oai" demo-oai-public-net
sudo service docker restart

```

## 2.2 OAI CN5G Setup

```bash
# Git oai-cn5g-fed repository
git clone https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed.git ~/oai-cn5g-fed
cd ~/oai-cn5g-fed
git checkout master
./scripts/syncComponents.sh --nrf-branch develop --amf-branch develop --smf-branch develop --spgwu-tiny-branch develop --ausf-branch develop --udm-branch develop --udr-branch develop --upf-vpp-branch develop --nssf-branch develop

docker pull oaisoftwarealliance/oai-amf:develop
docker pull oaisoftwarealliance/oai-nrf:develop
docker pull oaisoftwarealliance/oai-smf:develop
docker pull oaisoftwarealliance/oai-udr:develop
docker pull oaisoftwarealliance/oai-udm:develop
docker pull oaisoftwarealliance/oai-ausf:develop
docker pull oaisoftwarealliance/oai-upf-vpp:develop
docker pull oaisoftwarealliance/oai-spgwu-tiny:develop
docker pull oaisoftwarealliance/oai-nssf:develop

docker image tag oaisoftwarealliance/oai-amf:develop oai-amf:develop
docker image tag oaisoftwarealliance/oai-nrf:develop oai-nrf:develop
docker image tag oaisoftwarealliance/oai-smf:develop oai-smf:develop
docker image tag oaisoftwarealliance/oai-udr:develop oai-udr:develop
docker image tag oaisoftwarealliance/oai-udm:develop oai-udm:develop
docker image tag oaisoftwarealliance/oai-ausf:develop oai-ausf:develop
docker image tag oaisoftwarealliance/oai-upf-vpp:develop oai-upf-vpp:develop
docker image tag oaisoftwarealliance/oai-spgwu-tiny:develop oai-spgwu-tiny:develop
docker image tag oaisoftwarealliance/oai-nssf:develop oai-nssf:develop
```

## 2.3 OAI CN5G Configuration files
Download and copy the configuration files to ~/oai-cn5g-fed/docker-compose:
- [docker-compose-basic-nrf.yaml](tutorial_resources/docker-compose-basic-nrf.yaml)
- [oai_db.sql](tutorial_resources/oai_db.sql)

Change permissions on oai_db.sql to prevent mysql permission denied error:
```bash
chmod 644 ~/oai-cn5g-fed/docker-compose/oai_db.sql
```

## 2.4 SIM Card
Program SIM Card with [Open Cells Project](https://open-cells.com/) application [uicc-v2.6](https://open-cells.com/d5138782a8739209ec5760865b1e53b0/uicc-v2.6.tgz).

```bash
sudo ./program_uicc --adm 12345678 --imsi 208990000000001 --isdn 00000001 --acc 0001 --key fec86ba6eb707ed08905757b1bb44b8f --opc C42449363BBAD02B66D16BC975D77CC1 -spn "OpenAirInterface" --authenticate
```


# 3. OAI gNB

## 3.1 OAI gNB pre-requisites

### Build UHD from source
```bash
sudo apt install -y libboost-all-dev libusb-1.0-0-dev doxygen python3-docutils python3-mako python3-numpy python3-requests python3-ruamel.yaml python3-setuptools cmake build-essential

git clone https://github.com/EttusResearch/uhd.git ~/uhd
cd ~/uhd
git checkout v4.0.0.0
cd host
mkdir build
cd build
cmake ../
make -j 4
make test # This step is optional
sudo make install
sudo ldconfig
sudo uhd_images_downloader
```

## 3.2 Build OAI gNB

```bash
# Get openairinterface5g source code
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g
git checkout develop

# Install dependencies in Ubuntu 20.04
cd ~/openairinterface5g
source oaienv
cd cmake_targets
./build_oai -I

# Build OAI gNB
cd ~/openairinterface5g
source oaienv
cd cmake_targets
./build_oai -w USRP --nrUE --gNB --build-lib all -c
```


# 4. Run OAI CN5G and OAI gNB with USRP B210

## 4.1 Run OAI CN5G

```bash
cd ~/oai-cn5g-fed/docker-compose
python3 core-network.py --type start-basic --fqdn yes --scenario 1
```

## 4.2 Run OAI gNB

```bash
cd ~/openairinterface5g
source oaienv
cd cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --sa -E --continuous-tx
```

Make sure that during USRP initialization, it shows that USB 3 is used.

# 5. Testing with Quectel RM500Q

## 5.1 Setup Quectel
With [PuTTY](https://the.earth.li/~sgtatham/putty/latest/w64/putty.exe), send the following AT commands to the module using a serial interface (ex: COM2) at 115200 bps:
```bash
# MUST be sent at least once everytime there is a firmware upgrade!
AT+QMBNCFG="Select","ROW_Commercial"
AT+QMBNCFG="AutoSel",0
AT+CFUN=1,1
AT+CGDCONT=1
AT+CGDCONT=2
AT+CGDCONT=3
AT+CGDCONT=1,"IP","oai"

# (Optional, debug only, AT commands) Activate PDP context, retrieve IP address and test with ping
AT+CGACT=1,1
AT+CGPADDR=1
AT+QPING=1,"openairinterface.org"
```

## 5.2 Ping test
- UE host
```bash
ping 192.168.70.135 -n 1000 -S 12.1.1.2
```
- CN5G host
```bash
docker exec -it oai-ext-dn ping 12.1.1.2
```

## 5.3 Downlink iPerf test
- UE host
    - Download iPerf for Microsoft Windows from [here](https://iperf.fr/download/windows/iperf-2.0.9-win64.zip).
    - Extract to Desktop and run with Command Prompt:
```bash
cd C:\Users\User\Desktop\iPerf\iperf-2.0.9-win64\iperf-2.0.9-win64
iperf -s -u -i 1 -B 12.1.1.2
```

- CN5G host
```bash
docker exec -it oai-ext-dn iperf -u -t 86400 -i 1 -fk -B 192.168.70.135 -b 125M -c 12.1.1.2
```
