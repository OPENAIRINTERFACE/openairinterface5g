#!/bin/bash
: ${1?"Usage: $0 [-g num_gnb] [-n num_rsc] [-f filename]"}

# Usage info
show_help()
{
echo "
        Usage with script arguments: [-g num_gnb] [-n num_rsc] [-f filename] 

        -g num_gnb	Number of active gNBs
        -n num_rsc      Number of PRS resources
	-f filename     T tracer recorded .raw filename

        For Example: ./extract_prs_dumps.sh -g 1 -n 8 -f fr2_64_prbs.raw

        -h              Help
"
exit 0
}

while getopts g:n:f:h flag
do
   case "${flag}" in
      g) num_gnb=${OPTARG};;
      n) num_rsc=${OPTARG};;
      f) file=${OPTARG};;
      h) show_help;;
      *) exit -1;;
   esac
done
echo "num_gnb: $num_gnb";
echo "num_rsc: $num_rsc";
echo "filename: $file";

for (( i = 0; i < $num_gnb; i++ ))
do
   for (( j = 0; j < $num_rsc; j++ ))
   do
      name=chF_gnb${i}_${j}.raw
      echo "Extracting $name"
      ./extract -d ../T_messages.txt $file UE_PHY_DL_CHANNEL_ESTIMATE_FREQ chestF_t -f eNB_ID $i -f rsc_id $j -o $name -count 100
      name=chT_gnb${i}_${j}.raw
      echo "Extracting $name"
      ./extract -d ../T_messages.txt $file UE_PHY_DL_CHANNEL_ESTIMATE chest_t -f eNB_ID $i -f rsc_id $j -o $name -count 100
   done
done
