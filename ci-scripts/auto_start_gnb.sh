#!/bin/bash
while true
do
  echo "gNB will be started automatically..."
  sleep 1
  sudo .././cmake_targets/ran_build/build/nr-softmodem -E -O ../targets/PROJECTS/GENERIC-LTE-EPC/CONF/testing_gnb.conf
done
