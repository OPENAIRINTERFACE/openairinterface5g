#!/bin/bash

#$1 programs to be killed and checked
ps -aux |grep -E -i $1| awk '{print $2}' | sudo xargs kill -9 

var=`ps -aux |grep -E -i $1`
echo $var
if [ -n "$var" ]; then echo 'Match found'; else echo 'Match not found' ;fi 

