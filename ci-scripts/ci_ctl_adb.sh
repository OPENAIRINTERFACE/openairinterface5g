#!/bin/bash

usage() {
  echo "usage: $0 <command> <id>"
  echo "available commands: initialize, attach, detach, terminate, check"
}

if [ $# -ne 2 ]; then
  usage
  exit 1
fi

cmd=$1
id=$2

flightmode_off() {
  set +x
  adb -s $id shell "/data/local/tmp/on"
}

flightmode_on() {
  set +x
  adb -s $id shell "/data/local/tmp/off"
}

initialize() {
  set +x
  adb -s $id shell "svc data enable"   # make sure data services are enabled
  flightmode_on
}

terminate() {
  echo "terminate: does nothing"
}

check() {
  declare -A service=( ["0"]="IN_SERVICE" ["1"]="OUT_OF_SERVICE" ["2"]="EMERGENCY_ONLY" ["3"]="RADIO_POWERED_OFF")
  declare -A data=( ["0"]="DISCONNECTED" ["1"]="CONNECTING" ["2"]="CONNECTED" ["3"]="SUSPENDED")
  serv_idx=$(adb -s $id shell "dumpsys telephony.registry" | sed -n 's/.*mServiceState=\([0-3]\).*/\1/p')
  data_idx=$(adb -s $id shell "dumpsys telephony.registry" | sed -n 's/.*mDataConnectionState=\([0-3]\).*/\1/p')
  data_reason=$(adb -s $id shell "dumpsys telephony.registry" | sed -n 's/.*mDataConnectionReason=\([0-9a-zA-Z_]\+\).*/\1/p')
  #echo "Status Check UE $id"
  echo "Service State: ${service[$serv_idx]}"
  echo "Data State:    ${data[$data_idx]}"
  echo "Data Reason:   ${data_reason}"
}

case "${cmd}" in
  initialize) initialize;;
  attach) flightmode_off;;
  detach) flightmode_on;;
  terminate) terminate;;
  check) check;;
  *) echo "Invalid command $cmd"; usage; exit 1;;
esac
