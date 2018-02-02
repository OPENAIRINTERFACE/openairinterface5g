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

# file build_oai.bash
# brief apply a traffic generator, send traffic and measure the performance of OAI
# OTG is mainly used for OASIM and D-ITG (or iperf) for LTE-SOFTMODEM
# author  Navid Nikaein 
# company Eurecom
# email:  navid.nikaein@eurecom.fr 
# date 2015

#!/bin/bash
################################
# include helper functions
################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
. $THIS_SCRIPT_PATH/build_helper


#######################################
# Default PARAMETERS
######################################
#general 

declare PERF_APP="OTG-OAISIM" # ITG, ITG_DECODE, PING, OTG, 
declare TEST_MODE=0
declare KEEP_LOG_FILESNUM_PKTS=0
declare OWD="rttm" 
declare DURATION=60000 # ms
declare NUM_PKTS=10000
declare KBYTES=1000 # KBYTES
declare START=0
declare DST="127.0.0.1"
declare DPORT="8999"
declare TP="UDP"
declare IDT_DIST="CONSTANT"
declare PS_DIST="CONSTANT"

#IDT
declare RATE=1000    # pkt / s
declare MIN_RATE=100
declare MAX_RATE=1000

# set paths to the required binaries and check if the required binaries are available  
ENB_CONFIG=$OPENAIR_DIR/targets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf
OAISIM_EXEC=$OPENAIR_DIR/cmake_targets/oaisim_noS1_build_oai/build/oaisim_nos1
BYPASSE_ITTI=0
OTGPLOT="$OPENAIR2_DIR/UTIL/OTG/OTGplot"
PS2PDF="ps2pdf"

print_help_perf(){
    echo_success "Name : perf_oai  generate traffic and evaluate the performance "
    echo_success "Usage: perf_oai  -l ITG "
    echo_success "-l | --perf-app               : Set Performance evaluation app: ITGS, ITGD,PING, OTG-OAISIM, OTG-CBA, (default OTG-OAISIM)"
    echo_success "-m | --owd                    : enable D-ITG one-way-delay meter (default disabled)"
    echo_success "-e | --duration               : set the duration of the experiment (default 60000ms)"
    echo_success "-i | --idt-dist               : set the distribution of the inter-departure time: CONSTANT, UNIFORM,EXPONENTIAL (default CONSTANT)"
    echo_success "-s | --ps-dist                : set the distribution of the inter-departure time (default CONSTANT, available options: UNIFORM,EXPONENTIAL)"
    echo_success "-d | --dst                    : set the destination address (default 127.0.0.1)"
    echo_success "-p | --dst-port               : set the destination address (default NONE)"
    echo_success "-t | --test                   : enable test mode to validate the functionality (default disabled)"
    echo_success "-k | --keep-log-file          : keep the log files (default disabled)"
    echo_success "-c | --enb-config             : set the eNB config path (default $ENB_CONFIG)"
    echo_success "-x | --oaisim-exec            : set oaisim exec path (default $OASIM_EXEC with nos1 and ITTI enabled)"
    echo_success "-b | --bypass-itti            : bypass ITTI interafce (default use itti)" 


}

################################
# run ITGSEND
################################

itg_send(){

    $SUDO apt-get install -y test_install_package d-itg
    
    LOG_FILE="itg_log_template.txt"
    touch results/${LOG_FILE} 
    
    if [ $TEST_MODE = 0 ] ; then 
	declare -a PS=(32 64 128 256 512 1024 1408)
	declare -a IDT=(1 5 10 50 100 1000)
    else 
	declare -a PS=(32 64)
	declare -a IDT=(1)
    fi 

    declare PS_LEN=${#PS[@]} 
    declare IDT_LEN=${#IDT[@]} 
#echo_info "PS_LEN is $PS_LEN, IDT_LEN is $IDT_LEN"

    declare i=0
    declare j=0
    declare e=0
    # send traffic 
    for idt in ${IDT[@]}; do
#  echo_info "IDT is set to $idt i is $i"
	if [ $i -lt $IDT_LEN ]; then 
	    let i++;
	fi
	
	if [ $IDT_DIST = "CONSTANT" ]; then 
	    IDT_OPT="-C $idt "
	fi
	if [ $IDT_DIST = "UNIFORM" ]; then 
	    IDT_OPT="-U $idt ${IDT[i]} "
	fi
	if [ $IDT_DIST = "EXPONENTIAL" ]; then 
	    IDT_OPT="-E $idt "
	fi
	
	for ps in ${PS[@]}; do
#      echo_info "PS is $ps j is $j"
	    if [ $j -lt  $PS_LEN ]; then 
		let j++;
	    fi
	    if [ $PS_DIST = "CONSTANT" ]; then 
		PS_OPT="-c $ps "
	    fi
	    if [ $PS_DIST = "UNIFORM" ]; then 
		PS_OPT="-u $ps ${PS[j]} "
	    fi
	    if [ $PS_DIST = "EXPONENTIAL" ]; then 
		PS_OPT="-e $ps "
	    fi
	    start=$(date +%s)
	    RECV_FILE="recv_log_${IDT_DIST}_idt${idt}_${PS_DIST}_ps${ps}_${OWD}_${TP}"
	    echo_info "Start test_$i.$j:: ITGSend -a $DST $IDT_OPT $PS_OPT -m $OWD -d $START -T $TP -t $DURATION -x $RECV_FILE -l "
	    echo "Start test_$i.$j:: ITGSend -a $DST $IDT_OPT $PS_OPT -m $OWD -d $START -T $TP -t $DURATION -x $RECV_FILE -l " >> results/${oai_exp_date} 
	    #sleep 1 
	    ITGSend -a $DST $IDT_OPT $PS_OPT -m $OWD -d $START -T $TP -t $DURATION -x $RECV_FILE -l
	    itg_status=$?
	    end=$(date +%s)
	    diff=$(( $end - $start ))
	    
	    if [ $itg_status = 0 ] ; then 
		echo_success "[$end] test_$i.$j passed"
		echo "[$end] test_$i.$j passed" >> ./results/$LOG_FILE
		STATUS="PASSED"	   
	    else
		let e++;
		echo_error "[$end] test_$i.$j failed :: D-ITG return exit code $itg_status"
		echo "[$end]test_$i.$j failed :: ITG return exit code $itg_status" >> ./results/$LOG_FILE
		STATUS="FAILED"
	    fi 
	   
	    echo_info "End test_$i.$j:: runtime: $diff"
	    echo  "End test_$i.$j:: runtime: $diff" >>  results/${oai_exp_date} 
	    
	done
	
    done     
   
}

itg_decode(){
  
    declare i=0
    declare j=0
    if [ $TEST_MODE = 0 ] ; then 
	declare -a PS=(32 64 128 256 512 1024 1408)
	declare -a IDT=(1 5 10 50 100 1000)
    else 
	declare -a PS=(32 64)
	declare -a IDT=(1)
    fi 

    for idt in ${IDT[@]}; do
	
	for ps in ${PS[@]}; do
	    RECV_FILE="recv_log_${IDT_DIST}_idt${idt}_${PS_DIST}_ps${ps}_${OWD}_${TP}"
	    OUTPUT_FILE="results_${IDT_DIST}_idt${idt}_${PS_DIST}_ps${ps}_${OWD}_${TP}.txt"
	    OCTAVE_FILE="results_${IDT_DIST}_idt${idt}_${PS_DIST}_ps${ps}_${OWD}_${TP}.."
	    
	    echo_info "decode test_$i.$j: ITGDec $RECV_FILE -v -t -l $OUTPUT_FILE -o $OCTAVE_FILE"
	    ITGDec $RECV_FILE -v -t -l $OUTPUT_FILE
	    
	done
      
  done      
           

}

ping_stats(){

    status="failed"
    failedhosts=""
  
# add ip / hostname separated by white space

if [ $TEST_MODE = 0 ]; then 
    declare COUNT=100
    declare -a HOSTS=($DST)
    declare -a PS=(64 768 2048 4096 8192)
    declare -a IDT=(1 .8 .4 .2)
else 
    declare COUNT=10
    declare -a HOSTS=(localhost)
    declare -a PS=(64 2048)
    declare -a IDT=(.5)
fi 


declare i=0
declare j=0
declare k=0

start_exp=$(date +%s)

for host in ${HOSTS[@]}; do 
    let i++;
    let j=0;
    OUTPUT_FILE="rtt_host${host}.csv"
    touch results/${OUTPUT_FILE}
    for idt in ${IDT[@]}; do
	let j++;
	let k=0;
	for ps in ${PS[@]}; do
	    let k++;
             # | grep 'received' | awk -F',' '{ print $2 }' | awk '{ print $1 }')
	    start=$(date +%s)
	    LOG_FILE="recv_log_host${host}_idt${idt}_ps${ps}.txt"
	    touch results/${LOG_FILE} 
	    echo_info "Start test_$i.$j.$k:: ping -c $COUNT -q -U $host -s $ps -i $idt"
	    echo "Start test_$i.$j.$k:: ping -c $COUNT -q -U $host -s $ps -i $idt" >> results/${oai_exp_date} 
	    $(ping -c $COUNT -q -U $host -s $ps -i $idt 1>&2  >> ./results/$LOG_FILE )
	    end=$(date +%s)
	    diff=$(( $end - $start ))
	    
	    count=$(cat ./results/$LOG_FILE   | awk -F, '/received/{print $2*1}')
	    latency=$(cat ./results/$LOG_FILE | tail -1 |cut -f2- -d=)
	    
	    MIN=$(cut -f1 -d/ <<< $latency)
	    AVG=$(cut -f2 -d/ <<< $latency)
	    MAX=$(cut -f3 -d/ <<< $latency)
	    MDEV=$(cut -f4 -d/ <<< $latency | cut -f1 -d" ")
	    UNIT=$(cut -f2 -d" " <<< $latency)
	    
	    echo_success "Latency:: count $count min $MIN avg $AVG max $MAX mdev $MDEV ($UNIT)"
	    echo "$idt;$ps;$COUNT;$count;$MIN;$AVG;$MAX;$MDEV;" >> ./results/$OUTPUT_FILE 
	    if [ $count -eq 0 ]; then
		failedhosts="$failedhosts $host"
		status="failed"
	    else 
		status="passed"
	    fi
	    echo_info "End test_$i.$j.$k:: runtime: $diff :: status $status "
	    echo  "End test_$i.$j.$k:: runtime: $diff :; status $status" >>  results/${oai_exp_date} 
	   
	done
    done
done
end_exp=$(date +%s)
diff_exp=$(( $end_exp - $start_exp ))
	  
let total_tests=i*j*k;
echo_info "total tests: $total_tests for a duration $diff_exp (s)"

}

oaisim_otg_stats(){

# install the required packages
    
    $SUDO apt-get install -y octave >> results/perf_log.txt 2>&1
  
    $SUDO pkill oaisim
    $SUDO pkill oaisim_nos1

    $SUDO rmmod nasmesh > /dev/null 2>&1
    
    echo_success "Bringup UE interface..."
    $SUDO insmod  $OPENAIR_DIR/targets/bin/nasmesh.ko
    sync

    if [ ! -f $OAISIM_EXEC ]; then 
	echo_info "3.1 compiling OAISIM ($OPENAIR_DIR/cmake_targets/build_oai --oaisim -c --noS1)"
	($OPENAIR_DIR/cmake_targets/build_oai --oaisim -c  --noS1 >> results/perf_log.txt 2>&1 )
	build_stats=$?
	if [ $build_stats != 0 ] ; then 
	    echo_error "$OAISIM_EXEC cannot be built, check results/perf_log.txt file"
	    exit $?
	fi 
    else 
	echo_info "ensure that OAISIM is not built with the S1 interface"
    fi
    
    if [ ! -f $OTGPLOT ]; then 
	echo_error "$OTGPLOT not found"
	exit $?
    fi 

# Set the default Parameters
    ABSTRACTION=1
#FRAME_TYPE=0 # FDD=0, TDD =1, 2,3,4,5,6
    AGGR_RESULT=1
    STATUS="PASSED"
    EXTRA_STATS=0
    declare NUM_UES=1
    declare TRAFFIC_DIRECTIONS=2 # DL and UL

    if [ $TEST_MODE = 0 ]; then 
	declare -a TEMPLATES=(120 121 122 123 124 125 126 127 128 129) 
	declare -a FRAME_TYPE=(0) # (0 3)
	declare -a METRICS=(latency jitter goodput)
	declare -a RB=(25 50 100)
    else 
	declare -a TEMPLATES=(120 122 125 128)
	declare -a FRAME_TYPE=(0)
	declare -a METRICS=(latency)
	declare -a RB=(25)
    fi 
    
    if [ $ABSTRACTION = 1 ]; then 
	OPT="-a "
    fi
    
    if [ $BYPASSE_ITTI = 0 ]; then 
	OPT="$OPT -O $ENB_CONFIG "
    fi
    
    declare num_cols=0
    let num_cols=(TRAFFIC_DIRECTIONS*NUM_UES)+1
    if [ $AGGR_RESULT = 1 ]; then 
	COLUMN="[$num_cols:$num_cols]"
    else
	COLUMN="[1:$num_cols]"
    fi 
    
    declare i=0
    declare j=0
    declare k=0
    declare e=0
    
    start_exp=$(date +%s)
    
    for template in ${TEMPLATES[@]}; do 
	let i++;
	let j=0;
	for frame  in ${FRAME_TYPE[@]}; do
	    let j++;
	    let k=0;
	    
	    if [ $frame = 0 ]; then 
		OPT_FT="-F "
	    else 
		OPT_FT="-C $frame "
	    fi 
	    
	    for rb in ${RB[@]}; do
		let k++;
		start=$(date +%s)
		LOG_FILE="oaisim_log_template${template}_frame${frame}_rb${rb}.txt"
		touch results/${LOG_FILE} 
		echo_info "[$start] Start test_$i.$j.$k:: $OAISIM_EXEC $OPT_FT $OPT -R $RB -c $template"
		echo "Start test_$i.$j.$k:: $OAISIM_EXEC $OPT_FT $OPT -R $RB -c $template" >> results/${oai_exp_date} 
	#sleep 1 
		$OAISIM_EXEC $OPT_FT $OPT -R $RB -c $template 1>&2  >> ./results/$LOG_FILE
	# store exit status 
		oai_status=$?
		end=$(date +%s)
		diff=$(( $end - $start ))
		
        #check the oaisim exit status 
		if [ $oai_status = 0 ] ; then 
		    echo_success "[$end] test_$i.$j.$k passed"
		    echo "[$end] test_$i.$j.$k passed" >> ./results/$LOG_FILE
		    STATUS="PASSED"	   
		else
		    let e++;
		    echo_error "[$end] test_$i.$j.$k failed :: OAISIM return exit code $oai_status"
		    echo "[$end]test_$i.$j.$k failed :: OAISIM return exit code $oai_status" >> ./results/$LOG_FILE
		    STATUS="FAILED"
		fi 
	    	
         # create the curves 
		for metric  in ${METRICS[@]}; do
		    if [ $metric = "goodput" ]; then 
			unit="(kB/s)"
		    else
			unit="(ms)"
		    fi 	
		    if [ -f /tmp/otg_${metric}.dat ]; then 
			export TITLE="Application $metric $unit"
			cp /tmp/otg_${metric}.dat ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat
			echo_info "$OTGPLOT ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat $COLUMN"
			echo "$OTGPLOT ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat $COLUMN"  >> results/${oai_exp_date} 
			$($OTGPLOT ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat $COLUMN 1>&2 >> ./results/$LOG_FILE )
			$($PS2PDF -dOptimize=true -dEmbedAllFonts=true ./otg-${metric}-template${template}-frame${frame}-rb${rb}.eps )
		# remove the first line of the file
			echo "$(tail -n+2 ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat)" > ./results/otg-${metric}-template${template}-frame${frame}-rb${rb}.dat
		    else 
			echo_error "file /tmp/otg_${metric}.dat does not exists"
		    fi 
		done 
		if [ $KEEP_LOG_FILESNUM_PKTS = 0 ]; then 
		    rm -f ./results/$LOG_FILE
		fi 
		mv *.eps ./results/
		mv *.pdf ./results/
		mv /tmp/otg.log ./results/otg-template${template}-frame${frame}-rb${rb}.log
		echo_info "End test_$i.$j.$k:: runtime: $diff :: status $STATUS"
		echo  "End test_$i.$j.$k:: runtime: $diff :: status $STATUS" >>  results/${oai_exp_date} 
		
	    	
	    done
	done 
    done

    end_exp=$(date +%s)
    diff_exp=$(( $end_exp - $start_exp ))
    
    let total_tests=i*j*k;
    echo_info "total tests: $total_tests for a duration $diff_exp (s) error ($e)"
    echo  "total tests: $total_tests for a duration $diff_exp (s) error ($e)" >>  results/${oai_exp_date} 
    
}


cba_otg_stats(){

    test_install_package octave

ABSTRACTION=1
FRAME_TYPE=0 # FDD=0, TDD =1
AGGR_RESULT=1
#OAISIM="$OPENAIR_TARGETS/bin/oaisim"
OAISIM="bin/oaisim.cba" # to compile: make cleanall; make Rel10=1 CBA=1 in targets/SIMU/USER
OTGPLOT="$OPENAIR2_DIR/UTIL/OTG/OTGplot"
PS2PDF="ps2pdf"
STATUS="PASSED"
EXTRA_STATS=0
declare NUM_UES=7
declare TRAFFIC_DIRECTIONS=1 # DL and UL

if [ $TEST_MODE = 0 ]; then 
    declare -a TEMPLATES=(120 121 122 123 124 125 126 127 128 129 130)
    declare -a CBA=(0 1 2 3 4)
    declare -a BACKOFF=(0 15 30 60 120)
    declare -a METRICS=(latency jitter goodput)
    declare -a RB=(25)
else 
    declare -a TEMPLATES=(125)
    declare -a CBA=(1)
    declare -a BACKOFF=(0 15 30 60 120)
    declare -a METRICS=(latency)
    declare -a RB=(25)
fi 

if [ ! -f $OAISIM_EXEC ]; then 
    echo_error "$OAISIM_EXEC not found"
    exit $?
fi 
if [ ! -f $OTGPLOT ]; then 
    echo_error "$OTGPLOT not found"
    exit $?
fi 

if [ $ABSTRACTION = 1 ]; then 
    OPT="-a "
fi
if [ $FRAME_TYPE = 0 ]; then 
    OPT="$OPT -F "
fi

declare num_cols=0
let num_cols=(TRAFFIC_DIRECTIONS*NUM_UES)+1
if [ $AGGR_RESULT = 1 ]; then 
    COLUMN="[$num_cols:$num_cols]"
else
    COLUMN="[1:$num_cols]"
fi 

declare collision=0
declare enb_cba_access=0
declare ue_cba_access=0
declare missed=0
declare unused=0

declare i=0
declare j=0
declare k=0
declare e=0

start_exp=$(date +%s)

STATS1="cba_stats1.txt"
touch results/${STATS1}
    
for template in ${TEMPLATES[@]}; do 
    let i++;
    let j=0;
    for group  in ${CBA[@]}; do
	let j++;
	let k=0;
	for backoff in ${BACKOFF[@]}; do
	    let k++;
	    start=$(date +%s)
	    LOG_FILE="oaisim_log_template${template}_group${group}_backoff${backoff}.txt"
	    touch results/${LOG_FILE} 
	    echo_info "[$start] Start test_$i.$j.$k:: $OAISIM_EXEC $OPT -w $group -R $RB --cba-backoff $backoff -c $template"
	    echo "Start test_$i.$j.$k:: $OAISIM_EXEC $OPT -w $group -R $RB --cba-backoff $backoff -c $template" >> results/${oai_exp_date} 
	#sleep 1 
	    $OAISIM_EXEC $OPT -w $group -R $RB --cba-backoff $backoff -c $template 1>&2  >> ./results/$LOG_FILE
	# store exit status 
	    oai_status=$?
	    end=$(date +%s)
	    diff=$(( $end - $start ))
	    
        #check the oaisim exit status 
	    if [ $oai_status = 0 ] ; then 
		echo_success "[$end] test_$i.$j.$k passed"
		echo "[$end] test_$i.$j.$k passed" >> ./results/$LOG_FILE
		STATUS="PASSED"	   
	    else
		let e++;
		echo_error "[$end] test_$i.$j.$k failed :: OAISIM return exit code $oai_status"
		echo "[$end]test_$i.$j.$k failed :: OAISIM return exit code $oai_status" >> ./results/$LOG_FILE
		STATUS="FAILED"
	    fi 
	
	    if [ $group -gt 0 ]; then 
	    
		let ue_cba_access=$(cat ./results/$LOG_FILE    | grep -c "CBA transmission oppurtunity" )
		let enb_cba_access=$(cat ./results/$LOG_FILE   | grep -c "schedule CBA access" )
		let missed=$(cat ./results/$LOG_FILE          | grep -c "wait for backoff to expire" )
		let unused=enb_cba_access-ue_cba_access;
		let collision=$(cat ./results/$LOG_FILE        | grep -c "first CBA collision detected" )

	    #frame, subframe, ue, group
	    #collision_stats=$(cat ./results/$LOG_FILE      | grep "collision"  | cut -d " " -f3,5,12,15 )
		if [ $EXTRA_STATS = 1 ]; then 
		    STATS2="cba_template_${template}_stats2.txt"
		    touch results/${STATS2}
  		    while read -r line
		    do
			SFN=$(cut -f1  -d " " <<< $line)
			SSFN=$(cut -f2 -d " " <<< $line)
			UEID=$(cut -f3  -d " " <<< $line)
			GPID=$(cut -f4  -d " " <<< $line)
		#echo_success "$SFN;$SSFN;$UEID;$GPID;"
			echo "$SFN;$SSFN;$UEID;$GPID;"  >> ./results/$STATS2
		    done < <(cat ./results/$LOG_FILE      | grep "CBA collision set SR for UE"  | cut -d " " -f3,5,13,16 )
		fi
		
		echo_success "CBA stats:: template $template;group $group; backoff $backoff; enb cba allocation $enb_cba_access; ue cba access $ue_cba_access; collision $collision; missed $missed; unused $unused;"
		echo "$template;$group;$backoff;$enb_cba_access;$ue_cba_access;$collision;$missed;$unused;" >> ./results/$STATS1
	    fi
	    
         # create the curves 
	    for metric  in ${METRICS[@]}; do
		if [ $metric = "goodput" ]; then 
		    unit="(kB/s)"
		else
		    unit="(ms)"
		fi 	
		if [ -f /tmp/otg_${metric}.dat ]; then 
		    export TITLE="Application $metric $unit"
		    cp /tmp/otg_${metric}.dat ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat
		    echo_info "$OTGPLOT ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat $COLUMN"
		    echo "$OTGPLOT ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat $COLUMN"  >> results/${oai_exp_date} 
		    $($OTGPLOT ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat $COLUMN 1>&2 >> ./results/$LOG_FILE )
		    $($PS2PDF -dOptimize=true -dEmbedAllFonts=true ./otg-${metric}-template${template}-group${group}-backoff${backoff}.eps )
		# remove the first line of the file
		    echo "$(tail -n+2 ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat)" > ./results/otg-${metric}-template${template}-group${group}-backoff${backoff}.dat
		else 
		    echo_error "file /tmp/otg_${metric}.dat does not exists"
		fi 
	    done 
	    if [ $KEEP_LOG_FILESNUM_PKTS = 0 ]; then 
		rm -f ./results/$LOG_FILE
	    fi 
	    mv *.eps ./results/
	    mv *.pdf ./results/
	    mv /tmp/otg.log ./results/otg-template${template}-group${group}-backoff${backoff}.log
	    echo_info "End test_$i.$j.$k:: runtime: $diff :: status $STATUS"
	    echo  "End test_$i.$j.$k:: runtime: $diff :: status $STATUS" >>  results/${oai_exp_date} 
	
	    # backoff not required when CBA is not used
	    if [ $group -eq 0 ]; then
		break;
	    fi 
	    
	done
    done 
done

end_exp=$(date +%s)
diff_exp=$(( $end_exp - $start_exp ))

let total_tests=i*j*k;
echo_info "total tests: $total_tests for a duration $diff_exp (s) error ($e)"
echo  "total tests: $total_tests for a duration $diff_exp (s) error ($e)" >>  results/${oai_exp_date} 

}


function main()
{

  ############## script params #####################

    until [ -z "$1" ]
    do
	case "$1" in
	    -c | --test)
		TEST_MODE=1;
		echo_info "enabling the test mode"
		shift;
		;;
	    -l | --perf-app) 
		PERF_APP=$2
		echo_info "Setting the performance evaluation APP to $PERF_APP"
		if [ $PERF_APP = "DITG" ]; then 
		    echo_info "you need to run "
		fi 
		shift 2;
		;;
	    -m | --owd)
		OWD="owdm"
		echo_info "setting D-ITG one-way-delay meter"
		shift;
		;;
	    -e | --duration)
		DURATION=$2
		echo_info "Setting the traffic duration to $DURATION"
		shift 2;
		;;
	    -n | --num-pkts) 
		NUM_PKTS=$2
		echo_info "Setting number of packets to $NUM_PKTS"
		shift 2;
		;;
	    -k | --keep-log-file)
		KEEP_LOG_FILESNUM_PKTS=1
		echo_info "Keep the log files"
		shift;
		;;
	    -i | --idt-dist)
		IDT_DIST=$2
		echo_info "setting IDT distribution to $IDT_DIST"
		shift 2;
		;;
	    -s | --ps-dist)
		PS_DIST=$2
		echo_info "setting PS distribution to $PS_DIST"
		shift 2;
		;;
	    -d | --dst)
		DST=$2
		echo_info "setting the destination address to $DST"
		shift 2;
		;;
	    -p | --dst-port) 
		DPORT=$2
		echo_info "setting the destination port to $DPORT"
		shift 2;
		;;
	    -c | --enb-config) 
		ENB_CONFIG=$2
		echo_info "setting the enb config file to $ENB_CONFIG"
		shift 2;
		;;	   
	    -x | --oaisim-exec) 
		OAISIM_EXEC=$2
		echo_info "setting the oaisim exec to $OAISIM_EXEC"
		shift 2;
		;;
	    -b | --bypass-itti)
		BYPASSE_ITTI=1
		shift;;
	    -h | --help)
		print_help_perf
		exit -1
		;;
	    *)   
		echo "Unknown option $1"
		break ;
		# unknown option
		;;
	esac
    done
    
    #####################
    # create a bin dir
    #####################
    echo_info "1. Creating the results dir ..." 
    #rm -rf results
    mkdir -m 777 -p results 
    
    exp_date=`date +%Y_%m_%d`
    exp_time=`date +%H_%M_%S`
    oai_exp_date="exp_date_${exp_date}"
    touch results/${oai_exp_date} 
    
    touch results/perf_log.txt
    chmod -f 777 results/perf_log.txt
    
    echo "start experiment at date $exp_date time $exp_time " >> results/${oai_exp_date} 
    
    ############################################
    # setting and printing OAI envs, we should check here
    ############################################
    
    echo_info "2. Setting the OAI PATHS ..."
    
    set_openair_env 
    cecho "OPENAIR_HOME    = $OPENAIR_HOME" $green
    cecho "OPENAIR1_DIR    = $OPENAIR1_DIR" $green
    cecho "OPENAIR2_DIR    = $OPENAIR2_DIR" $green
    cecho "OPENAIR3_DIR    = $OPENAIR3_DIR" $green
    cecho "OPENAIR3_DIR   = $OPENAIR3_DIR" $green
    cecho "OPENAIR_TARGETS = $OPENAIR_TARGETS" $green
    
    
    echo "OPENAIR_HOME    = $OPENAIR_HOME" >>  results/${oai_exp_date}
    echo "OPENAIR1_DIR    = $OPENAIR1_DIR"  >>  results/${oai_exp_date}
    echo "OPENAIR2_DIR    = $OPENAIR2_DIR"  >>  results/${oai_exp_date}
    echo "OPENAIR3_DIR    = $OPENAIR3_DIR"  >>  results/${oai_exp_date}
    echo "OPENAIR3_DIR   = $OPENAIR3_DIR"  >>  results/${oai_exp_date}
    echo "OPENAIR_TARGETS = $OPENAIR_TARGETS"  >>  results/${oai_exp_date}
    

    echo_info "eNB_CONFIG : " $ENB_CONFIG
    echo_info "OAISIM_EXEC PATH: " $OAISIM_EXEC
    echo_info "OTGPLOT PATH:" $OTGPLOT
    
    case "$PERF_APP" in
	'ITGS')
            echo_info "3. running ITGSend (ensure that either oaisim or lte-softmodem is running)"
            itg_send
            ;;
	'ITGD')
            echo_info "running ITGDECODE (At the end of the experiment, ensure that the results are in the current directory)"
            itg_decode
            ;;
	'PING')
	    echo_info "3. running ping (ensure that either oaisim or lte-softmodem is running)"
            ping_stats
    	    ;;
	'OTG-OAISIM')
	    echo_info "3. running OTG on oaisim (this will call oaisim as well)"
            oaisim_otg_stats
    	    ;;
	'OTG-CBA')
	    echo_info "3. running OTG on oaisim with cba"
            cba_otg_stats
    	    ;;
	'IPERF')
	    echo_warning "iperf not supported"
	    ;;
	'NONE')
            ;;
	*)
            echo_error "Unknown option $RUN_ITG"
            ;;
    esac

}

main "$@"

