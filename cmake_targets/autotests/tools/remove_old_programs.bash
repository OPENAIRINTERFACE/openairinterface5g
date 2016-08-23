#!/bin/bash

#$1 programs to be killed and checked
echo "removing old programs..."
echo "args = $1"
echo "programs to be killed"
ps -aux |grep -E -i $1

ps -aux |grep -E -i $1| awk '{print $2}' | sudo xargs kill -9 

var=`ps -aux |grep -E -i $1`
echo $var
if [ -n "$var" ]; then echo 'Match found'; else echo 'Match not found' ;fi 

