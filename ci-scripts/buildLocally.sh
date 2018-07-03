#!/bin/bash

function usage {
    echo "OAI Local Build Check script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    buildLocally.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --workspace #### OR -ws ####"
    echo "    Specify the workspace"
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

if [ $# -ne 2 ] && [ $# -ne 1 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    -ws|--workspace)
    JENKINS_WKSP="$2"
    shift
    shift
    ;;
    *)
    echo "Syntax Error: unknown option: $key"
    echo ""
    usage
    exit 1
esac
done

cd $JENKINS_WKSP
STATUS=0

############################################################
# Creating a tmp folder to store results and artifacts
############################################################
if [ ! -d $JENKINS_WKSP/archives ]
then
    mkdir $JENKINS_WKSP/archives
fi

source oaienv
cd $JENKINS_WKSP/cmake_targets

############################################################
# Building eNb with USRP option
############################################################
ARCHIVES_LOC=$JENKINS_WKSP/archives/enb_usrp
if [ ! -d $ARCHIVES_LOC ]
then
    mkdir $ARCHIVES_LOC
fi
./build_oai --eNB -w USRP -c

# Generated log files:
if [ -f $JENKINS_WKSP/cmake_targets/log/lte-softmodem.Rel14.txt ]
then
    LOCAL_STAT=`egrep -c "Built target lte-softmodem" $JENKINS_WKSP/cmake_targets/log/lte-softmodem.Rel14.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/lte-softmodem.Rel14.txt $ARCHIVES_LOC
else
    STATUS=-1
fi
if [ -f $JENKINS_WKSP/cmake_targets/log/params_libconfig.Rel14.txt ]
then
    LOCAL_STAT=`egrep -c "Built target params_libconfig" $JENKINS_WKSP/cmake_targets/log/params_libconfig.Rel14.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/params_libconfig.Rel14.txt $ARCHIVES_LOC
else
    STATUS=-1
fi
if [ -f $JENKINS_WKSP/cmake_targets/log/coding.Rel14.txt ]
then
    LOCAL_STAT=`egrep -c "Built target coding" $JENKINS_WKSP/cmake_targets/log/coding.Rel14.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/coding.Rel14.txt $ARCHIVES_LOC
else
    STATUS=-1
fi
if [ -f $JENKINS_WKSP/cmake_targets/log/oai_usrpdevif.Rel14.txt ]
then
    LOCAL_STAT=`egrep -c "Built target oai_usrpdevif" $JENKINS_WKSP/cmake_targets/log/oai_usrpdevif.Rel14.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/oai_usrpdevif.Rel14.txt $ARCHIVES_LOC
else
    STATUS=-1
fi

############################################################
# Building basic simulator
############################################################
ARCHIVES_LOC=$JENKINS_WKSP/archives/basic_sim
if [ ! -d $ARCHIVES_LOC ]
then
    mkdir $ARCHIVES_LOC
fi
cd $JENKINS_WKSP/cmake_targets
./build_oai --basic-simulator -c

# Generated log files:
if [ -f $JENKINS_WKSP/cmake_targets/log/basic_simulator_enb.txt ]
then
    LOCAL_STAT=`egrep -c "Built target lte-softmodem" $JENKINS_WKSP/cmake_targets/log/basic_simulator_enb.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/basic_simulator_enb.txt $ARCHIVES_LOC
else
    STATUS=-1
fi
if [ -f $JENKINS_WKSP/cmake_targets/log/basic_simulator_ue.txt ]
then
    LOCAL_STAT=`egrep -c "Built target lte-uesoftmodem" $JENKINS_WKSP/cmake_targets/log/basic_simulator_ue.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/basic_simulator_ue.txt $ARCHIVES_LOC
else
    STATUS=-1
fi
if [ -f $JENKINS_WKSP/cmake_targets/log/conf2uedata.Rel14.txt ]
then
    LOCAL_STAT=`egrep -c "Built target conf2uedata" $JENKINS_WKSP/cmake_targets/log/conf2uedata.Rel14.txt`
    if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
    cp $JENKINS_WKSP/cmake_targets/log/conf2uedata.Rel14.txt $ARCHIVES_LOC
else
    STATUS=-1
fi

############################################################
# Creating a zip for Jenkins archiving
############################################################
cd $JENKINS_WKSP/archives/
zip -r local_build_logs.zip basic_sim enb_usrp

exit $STATUS
