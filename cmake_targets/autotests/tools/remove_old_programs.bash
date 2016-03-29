#!/bin/bash

#$1 programs to be killed and checked
var=`ps -A |grep -E -i $1`
echo $var
if [ -n "$var" ]; then echo 'Match found'; else echo 'Match not found' ;fi 

