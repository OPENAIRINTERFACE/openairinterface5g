#!/bin/bash
#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */

# \author Navid Nikaein, Rohit Gupta

# To free unused memory else test setup runs out of memory

mem_threshold=0.2 #If free memory is less than this threshold, then VM drop cache is called
mem_tot=`vmstat -s -S k |grep "total memory" | awk '{print $1}'`
mem_free=`vmstat -s -S k |grep "free memory" | awk '{print $1}'`

mem_frac=`bc <<< "scale=4;$mem_free/$mem_tot"`
echo $mem_frac
#mem_frac=`bc <<< "scale=4;`echo $mem_free`/`echo $mem_tot`"`
echo "Total Memory = $mem_tot k "
echo "Free Memory = $mem_free k"
echo "Fraction free memory = $mem_frac "

res=`bc <<< "$mem_frac < 0.2" `

echo "Comparison Result = $res"

if [ "$res" == "1" ]
then
  echo "Free memory less than threshold = $mem_threshold"
  sudo -E bash -c 'echo 3 > /proc/sys/vm/drop_caches ' 
fi


