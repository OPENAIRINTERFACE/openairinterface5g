set -x

sudo mbimcli -p -d /dev/cdc-wdm0 --set-radio-state=off
sleep 1
sudo mbimcli -p -d /dev/cdc-wdm0 --set-radio-state=on
sleep 2
sudo mbimcli -p -d /dev/cdc-wdm0 --attach-packet-service

sudo mbimcli -p -d /dev/cdc-wdm0 --connect=session-id=0,access-string=oai.ipv4,ip-type=ipv4
sudo /opt/mbim/mbim-set-ip.sh /dev/cdc-wdm0 wwan0 0
