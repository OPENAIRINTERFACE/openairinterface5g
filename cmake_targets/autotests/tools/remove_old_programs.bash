#!/bin/bash

#$1 programs to be killed and checked
echo "removing old programs..."
echo "args = $1"
echo "script name = $0"
filename=$(basename "$0")
echo "programs to be killed"
echo "bash PID = $$"
pid='$$'
#we need to remove current program and grip as we kill ourselves otherwise :)
var=`ps -aux |grep -E -i $1 | awk '{print $2}'`

echo $var

echo "$var" | sed 's/'$$'/ /' | sudo xargs kill -9

var=`ps -aux |grep -E -i $1| grep -E -v '$filename|grep|$$'`
echo $var
if [ -n "$var" ]; then echo 'Match found'; else echo 'Match not found' ;fi 

