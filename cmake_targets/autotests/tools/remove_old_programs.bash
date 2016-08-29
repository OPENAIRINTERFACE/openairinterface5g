#!/bin/bash

#$1 programs to be killed and checked
echo "removing old programs..."
echo "args = $1"
echo "script name = $0"
filename=$(basename "$0")
echo "filename = $filename"
echo "programs to be killed...$1"

echo "bash PID = $$" 
pid="$$"
echo "pid = $pid"

echo "Killing programs now..."

ps -aux |grep -E -i -w "$1"

var=`ps -aux |grep -E -i -w "$1" |awk '{print $2}'| tr '\n' ' ' | sed  "s/$pid/ /"`
echo "Killing processes...$var"
#var=`ps -aux |grep -E -i '$1' |awk '{print $2}'| tr '\n' ' ' | sed  "s/$pid/ /"`
#echo $var 
if [ -n "$var" ] ; then  sudo  kill -9 $var ; fi

#| sudo xargs kill -9 

echo "checking for old programs..."
var=`ps -aux |grep -E -i -w '$1' |grep -Ev 'grep' | grep -Ev '$filename'`

echo $var
if [ -n "$var" ]; then echo 'Match found'; else echo 'Match not found' ;fi 

