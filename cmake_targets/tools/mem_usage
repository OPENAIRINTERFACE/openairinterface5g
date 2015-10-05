#!/usr/bin/env bash

pid=$1
sleep_sec=0.5

if [ "$2" != "" ] ; then
sleep_sec=$2
fi
#declare -r cnt=$sleep_sec
OUTPUT_FILE="/tmp/mem_usage_${pid}_${sleep_sec}.csv" 
echo "RSS(KB);PSS(KB);SHARED_CLEAN(KB);SHARED_DIRTY(KB);PRIVATE)KB)" >> $OUTPUT_FILE 
echo "Mem usage for PID $pid with observation period of ${sleep_sec}" 

while ps $pid >/dev/null
do
    if [ -f /proc/$pid/smaps ]; then
        rss=$(awk '/^Rss/ {i = i + $2} END {print i}' /proc/$pid/smaps)
        pss=$(awk '/^Pss/ {i = i + $2 + 0.5} END {print i}' /proc/$pid/smaps)
        sc=$(awk '/^Shared_Clean/ {i = i + $2} END {print i}' /proc/$pid/smaps)            
        sd=$(awk '/^Shared_Dirty/ {i = i + $2} END {print i}' /proc/$pid/smaps)
        pc=$(awk '/^Private_Clean/ {i = i + $2} END {print i}' /proc/$pid/smaps)
        pd=$(awk '/^Private_Dirty/ {i = i + $2} END {print i}' /proc/$pid/smaps)
	echo "$cnt;$rss;$pss;$sc;$pc;$pd;" >> $OUTPUT_FILE 
    fi 
   
    sleep $sleep_sec
done 
