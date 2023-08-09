#!/bin/bash

# use UP and DOWN arrow keys to scroll the view displayed by timeplot

while read -n 1 key
do
  case "$key" in
  'B' )
    kill -SIGUSR1 `ps aux|grep timeplot|grep -v grep|grep -v sh|tr -s ' ' :|cut -f 2 -d :`
  ;;
  'A' )
    kill -SIGUSR2 `ps aux|grep timeplot|grep -v grep|grep -v sh|tr -s ' ' :|cut -f 2 -d :`
  ;;
  esac
done
