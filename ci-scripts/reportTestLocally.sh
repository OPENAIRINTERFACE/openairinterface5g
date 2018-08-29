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

function usage {
    echo "OAI Test Report script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo ""
    echo "    reportTestLocally.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
    echo "Job Options:"
    echo "------------"
    echo ""
    echo "    --git-url #### OR -gu ####"
    echo "    Specify the URL of the GIT Repository."
    echo ""
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
    echo ""
    echo "    --trigger merge-request OR -mr"
    echo "    --trigger push          OR -pu"
    echo "    Specify trigger action of the Jenkins job. Either a merge-request event or a push event."
    echo ""
    echo "Merge-Request Options:"
    echo "----------------------"
    echo ""
    echo "    --src-branch #### OR -sb ####"
    echo "    Specify the source branch of the merge request."
    echo ""
    echo "    --src-commit #### OR -sc ####"
    echo "    Specify the source commit ID (SHA-1) of the merge request."
    echo ""
    echo "    --target-branch #### OR -tb ####"
    echo "    Specify the target branch of the merge request (usually develop)."
    echo ""
    echo "    --target-commit #### OR -tc ####"
    echo "    Specify the target commit ID (SHA-1) of the merge request."
    echo ""
    echo "Push Options:"
    echo "----------------------"
    echo ""
    echo "    --branch #### OR -br ####"
    echo "    Specify the branch of the push event."
    echo ""
    echo "    --commit #### OR -co ####"
    echo "    Specify the commit ID (SHA-1) of the push event."
    echo ""
    echo ""
}

function trigger_usage {
    echo "OAI Test Report script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "    --trigger merge-request OR -mr"
    echo "    --trigger push          OR -pu"
    echo "    Specify trigger action of the Jenkins job. Either a merge-request event or a push event."
    echo ""
}

jb_checker=0
mr_checker=0
pu_checker=0
MR_TRIG=0
PU_TRIG=0
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    -gu|--git-url)
    GIT_URL="$2"
    let "jb_checker|=0x1"
    shift
    shift
    ;;
    -jn|--job-name)
    JOB_NAME="$2"
    let "jb_checker|=0x2"
    shift
    shift
    ;;
    -id|--build-id)
    BUILD_ID="$2"
    let "jb_checker|=0x4"
    shift
    shift
    ;;
    --trigger)
    TRIG="$2"
    case $TRIG in
        merge-request)
        MR_TRIG=1
        ;;
        push)
        PU_TRIG=1
        ;;
        *)
        echo ""
        echo "Syntax Error: Invalid Trigger option -> $TRIG"
        echo ""
        trigger_usage
        exit
        ;;
    esac
    let "jb_checker|=0x8"
    shift
    shift
    ;;
    -mr)
    MR_TRIG=1
    let "jb_checker|=0x8"
    shift
    ;;
    -pu)
    PU_TRIG=1
    let "jb_checker|=0x8"
    shift
    ;;
    -sb|--src-branch)
    SOURCE_BRANCH="$2"
    let "mr_checker|=0x1"
    shift
    shift
    ;;
    -sc|--src-commit)
    SOURCE_COMMIT_ID="$2"
    let "mr_checker|=0x2"
    shift
    shift
    ;;
    -tb|--target-branch)
    TARGET_BRANCH="$2"
    let "mr_checker|=0x4"
    shift
    shift
    ;;
    -tc|--target-commit)
    TARGET_COMMIT_ID="$2"
    let "mr_checker|=0x8"
    shift
    shift
    ;;
    -br|--branch)
    SOURCE_BRANCH="$2"
    let "pu_checker|=0x1"
    shift
    shift
    ;;
    -co|--commit)
    SOURCE_COMMIT_ID="$2"
    let "pu_checker|=0x2"
    shift
    shift
    ;;
    *)
    echo "Syntax Error: unknown option: $key"
    echo ""
    usage
    exit 1
    ;;
esac

done

if [ $jb_checker -ne 15 ]
then
    echo ""
    echo "Syntax Error: missing job information."
    # TODO : list missing info
    echo ""
    exit 1
fi

if [ $PU_TRIG -eq 1 ] && [ $MR_TRIG -eq 1 ]
then
    echo ""
    echo "Syntax Error: trigger action incoherent."
    echo ""
    trigger_usage
    exit 1
fi

if [ $PU_TRIG -eq 1 ]
then
    if [ $pu_checker -ne 3 ]
    then
        echo ""
        echo "Syntax Error: missing push information."
        # TODO : list missing info
        echo ""
        exit 1
    fi
fi

if [ $MR_TRIG -eq 1 ]
then
    if [ $mr_checker -ne 15 ]
    then
        echo ""
        echo "Syntax Error: missing merge-request information."
        # TODO : list missing info
        echo ""
        exit 1
    fi
fi

echo "<!DOCTYPE html>" > ./test_simulator_results.html
echo "<html class=\"no-js\" lang=\"en-US\">" >> ./test_simulator_results.html
echo "<head>" >> ./test_simulator_results.html
echo "  <title>Simulator Results for $JOB_NAME job build #$BUILD_ID</title>" >> ./test_simulator_results.html
echo "  <base href = \"http://www.openairinterface.org/\" />" >> ./test_simulator_results.html
echo "</head>" >> ./test_simulator_results.html
echo "<body>" >> ./test_simulator_results.html
echo "  <table style=\"border-collapse: collapse; border: none;\">" >> ./test_simulator_results.html
echo "    <tr style=\"border-collapse: collapse; border: none;\">" >> ./test_simulator_results.html
echo "      <td style=\"border-collapse: collapse; border: none;\">" >> ./test_simulator_results.html
echo "        <a href=\"http://www.openairinterface.org/\">" >> ./test_simulator_results.html
echo "           <img src=\"/wp-content/uploads/2016/03/cropped-oai_final_logo2.png\" alt=\"\" border=\"none\" height=50 width=150>" >> ./test_simulator_results.html
echo "           </img>" >> ./test_simulator_results.html
echo "        </a>" >> ./test_simulator_results.html
echo "      </td>" >> ./test_simulator_results.html
echo "      <td style=\"border-collapse: collapse; border: none; vertical-align: center;\">" >> ./test_simulator_results.html
echo "        <b><font size = \"6\">Job Summary -- Job: $JOB_NAME -- Build-ID: $BUILD_ID</font></b>" >> ./test_simulator_results.html
echo "      </td>" >> ./test_simulator_results.html
echo "    </tr>" >> ./test_simulator_results.html
echo "  </table>" >> ./test_simulator_results.html
echo "  <br>" >> ./test_simulator_results.html
echo "   <table border = \"1\">" >> ./test_simulator_results.html
echo "      <tr>" >> ./test_simulator_results.html
echo "        <td bgcolor = \"lightcyan\" >GIT Repository</td>" >> ./test_simulator_results.html
echo "        <td><a href=\"$GIT_URL\">$GIT_URL</a></td>" >> ./test_simulator_results.html
echo "      </tr>" >> ./test_simulator_results.html
echo "      <tr>" >> ./test_simulator_results.html
echo "        <td bgcolor = \"lightcyan\" >Job Trigger</td>" >> ./test_simulator_results.html
if [ $PU_TRIG -eq 1 ]; then echo "        <td>Push Event</td>" >> ./test_simulator_results.html; fi
if [ $MR_TRIG -eq 1 ]; then echo "        <td>Merge-Request</td>" >> ./test_simulator_results.html; fi
echo "      </tr>" >> ./test_simulator_results.html
if [ $PU_TRIG -eq 1 ]
then
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Branch</td>" >> ./test_simulator_results.html
    echo "        <td>$SOURCE_BRANCH</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Commit ID</td>" >> ./test_simulator_results.html
    echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
fi
if [ $MR_TRIG -eq 1 ]
then
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Source Branch</td>" >> ./test_simulator_results.html
    echo "        <td>$SOURCE_BRANCH</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Source Commit ID</td>" >> ./test_simulator_results.html
    echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Target Branch</td>" >> ./test_simulator_results.html
    echo "        <td>$TARGET_BRANCH</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" >Target Commit ID</td>" >> ./test_simulator_results.html
    echo "        <td>$TARGET_COMMIT_ID</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
fi
echo "   </table>" >> ./test_simulator_results.html
echo "   <h2>Test Summary</h2>" >> ./test_simulator_results.html

ARCHIVES_LOC=archives/basic_sim/test
if [ -d $ARCHIVES_LOC ]
then
    echo "   <h3>Basic Simulator Check</h3>" >> ./test_simulator_results.html

    echo "   <table border = \"1\">" >> ./test_simulator_results.html
    echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
    echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
    echo "        <th>Command</th>" >> ./test_simulator_results.html
    echo "        <th>Status</th>" >> ./test_simulator_results.html
    echo "        <th>Statistics</th>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html

    PING_TESTS=`ls $ARCHIVES_LOC/ping*txt`
    for PING_CASE in $PING_TESTS
    do
        echo "      <tr>" >> ./test_simulator_results.html
        NAME=`echo $PING_CASE | sed -e "s#$ARCHIVES_LOC/##"`
        echo "        <td>$NAME</td>" >> ./test_simulator_results.html
        CMD=`egrep "COMMAND IS" $PING_CASE | sed -e "s#COMMAND IS: ##"`
        echo "        <td>$CMD</td>" >> ./test_simulator_results.html
        FILE_COMPLETE=`egrep -c "ping statistics" $PING_CASE`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            echo "        <td>N/A</td>" >> ./test_simulator_results.html
        else
            NB_TR_PACKETS=`egrep "packets transmitted" $PING_CASE | sed -e "s# packets transmitted.*##"`
            NB_RC_PACKETS=`egrep "packets transmitted" $PING_CASE | sed -e "s#^.*packets transmitted, ##" -e "s# received,.*##"`
            if [ $NB_TR_PACKETS -eq $NB_RC_PACKETS ]
            then
                echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
            else
                echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            fi
            echo "        <td>" >> ./test_simulator_results.html
            echo "            <pre>" >> ./test_simulator_results.html
            STATS=`egrep "packets transmitted" $PING_CASE | sed -e "s#^.*received, ##" -e "s#, time.*##" -e "s# packet loss##"`
            echo "Packet Loss : $STATS" >> ./test_simulator_results.html
            RTTMIN=`egrep "rtt min" $PING_CASE | awk '{split($4,a,"/"); print a[1] " " $5}'`
            echo "RTT Minimal : $RTTMIN" >> ./test_simulator_results.html
            RTTAVG=`egrep "rtt min" $PING_CASE | awk '{split($4,a,"/"); print a[2] " " $5}'`
            echo "RTT Average : $RTTAVG" >> ./test_simulator_results.html
            RTTMAX=`egrep "rtt min" $PING_CASE | awk '{split($4,a,"/"); print a[3] " " $5}'`
            echo "RTT Maximal : $RTTMAX" >> ./test_simulator_results.html
            echo "            </pre>" >> ./test_simulator_results.html
            echo "        </td>" >> ./test_simulator_results.html
        fi
        echo "      </tr>" >> ./test_simulator_results.html
    done

    IPERF_TESTS=`ls $ARCHIVES_LOC/iperf*client*txt`
    for IPERF_CASE in $IPERF_TESTS
    do
        echo "      <tr>" >> ./test_simulator_results.html
        NAME=`echo $IPERF_CASE | sed -e "s#$ARCHIVES_LOC/##"`
        echo "        <td>$NAME</td>" >> ./test_simulator_results.html
        CMD=`egrep "COMMAND IS" $IPERF_CASE | sed -e "s#COMMAND IS: ##"`
        echo "        <td>$CMD</td>" >> ./test_simulator_results.html
        FILE_COMPLETE=`egrep -c "Server Report" $IPERF_CASE`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            echo "        <td>N/A</td>" >> ./test_simulator_results.html
        else
            REQ_BITRATE=`echo $CMD | sed -e "s#^.*-b ##" -e "s#-i 1.*##"`
            if [[ $REQ_BITRATE =~ .*K.* ]]
            then
                REQ_BITRATE=`echo $REQ_BITRATE | sed -e "s#K##"`
                FLOAT_REQ_BITRATE=`echo "$REQ_BITRATE * 1000.0" | bc -l`
            fi
            if [[ $REQ_BITRATE =~ .*M.* ]]
            then
                REQ_BITRATE=`echo $REQ_BITRATE | sed -e "s#M##"`
                FLOAT_REQ_BITRATE=`echo "$REQ_BITRATE * 1000000.0" | bc -l`
            fi
            if [[ $REQ_BITRATE =~ .*G.* ]]
            then
                REQ_BITRATE=`echo $REQ_BITRATE | sed -e "s#G##"`
                FLOAT_REQ_BITRATE=`echo "$REQ_BITRATE * 1000000000.0" | bc -l`
            fi
            EFFECTIVE_BITRATE=`tail -n3 $IPERF_CASE | egrep "Mbits/sec" | sed -e "s#^.*MBytes *##" -e "s#sec.*#sec#"`
            if [[ $EFFECTIVE_BITRATE =~ .*Kbits/sec.* ]]
            then
                EFFECTIVE_BITRATE=`echo $EFFECTIVE_BITRATE | sed -e "s# *Kbits/sec.*##"`
                FLOAT_EFF_BITRATE=`echo "$EFFECTIVE_BITRATE * 1000" | bc -l`
            fi
            if [[ $EFFECTIVE_BITRATE =~ .*Mbits/sec.* ]]
            then
                EFFECTIVE_BITRATE=`echo $EFFECTIVE_BITRATE | sed -e "s# *Mbits/sec.*##"`
                FLOAT_EFF_BITRATE=`echo "$EFFECTIVE_BITRATE * 1000000" | bc -l`
            fi
            if [[ $EFFECTIVE_BITRATE =~ .*Gbits/sec.* ]]
            then
                EFFECTIVE_BITRATE=`echo $EFFECTIVE_BITRATE | sed -e "s# *Gbits/sec.*##"`
                FLOAT_EFF_BITRATE=`echo "$EFFECTIVE_BITRATE * 1000000000" | bc -l`
            fi
            PERF=`echo "100 * $FLOAT_EFF_BITRATE / $FLOAT_REQ_BITRATE" | bc -l | awk '{printf "%.2f", $0}'`
            PERF_INT=`echo "100 * $FLOAT_EFF_BITRATE / $FLOAT_REQ_BITRATE" | bc -l | awk '{printf "%.0f", $0}'`
            if [[ $PERF_INT -lt 90 ]]
            then
                echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            else
                echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
            fi
            echo "        <td>" >> ./test_simulator_results.html
            echo "            <pre>" >> ./test_simulator_results.html
            EFFECTIVE_BITRATE=`tail -n3 $IPERF_CASE | egrep "Mbits/sec" | sed -e "s#^.*MBytes *##" -e "s#sec.*#sec#"`
            echo "Bitrate      : $EFFECTIVE_BITRATE" >> ./test_simulator_results.html
            echo "Bitrate Perf : $PERF %" >> ./test_simulator_results.html
            JITTER=`tail -n3 $IPERF_CASE | egrep "Mbits/sec" | sed -e "s#^.*Mbits/sec *##" -e "s#ms.*#ms#"`
            echo "Jitter       : $JITTER" >> ./test_simulator_results.html
            PACKETLOSS=`tail -n3 $IPERF_CASE | egrep "Mbits/sec" | sed -e "s#^.*(##" -e "s#).*##"`
            echo "Packet Loss  : $PACKETLOSS" >> ./test_simulator_results.html
            echo "            </pre>" >> ./test_simulator_results.html
            echo "        </td>" >> ./test_simulator_results.html
        fi
        echo "      </tr>" >> ./test_simulator_results.html
    done

    echo "   </table>" >> ./test_simulator_results.html
fi

ARCHIVES_LOC=archives/phy_sim/test
if [ -d $ARCHIVES_LOC ]
then
    echo "   <h3>Physical Simulators Check</h3>" >> ./test_simulator_results.html

    echo "   <table border = \"1\">" >> ./test_simulator_results.html
    echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
    echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
    echo "        <th>Nb Tests</th>" >> ./test_simulator_results.html
    echo "        <th>Nb Errors</th>" >> ./test_simulator_results.html
    echo "        <th>Nb Failures</th>" >> ./test_simulator_results.html
    echo "        <th>Nb Failures</th>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html

    XML_TESTS=`ls $ARCHIVES_LOC/*xml`
    for XML_FILE in $XML_TESTS
    do
        echo "      <tr>" >> ./test_simulator_results.html
        NAME=`echo $XML_FILE | sed -e "s#$ARCHIVES_LOC/##"`
        NB_TESTS=`egrep "testsuite errors" $XML_FILE | sed -e "s#^.*tests='##" -e "s#' *time=.*##"`
        NB_ERRORS=`egrep "testsuite errors" $XML_FILE | sed -e "s#^.*errors='##" -e "s#' *failures=.*##"`
        NB_FAILURES=`egrep "testsuite errors" $XML_FILE | sed -e "s#^.*failures='##" -e "s#' *hostname=.*##"`
        NB_SKIPPED=`egrep "testsuite errors" $XML_FILE | sed -e "s#^.*skipped='##" -e "s#' *tests=.*##"`
        if [ $NB_ERRORS -eq 0 ] && [ $NB_FAILURES -eq 0 ]
        then
            echo "        <td bgcolor = \"green\" >$NAME</td>" >> ./test_simulator_results.html
        else
            echo "        <td bgcolor = \"red\" >$NAME</td>" >> ./test_simulator_results.html
        fi
        echo "        <td>$NB_TESTS</td>" >> ./test_simulator_results.html
        echo "        <td>$NB_ERRORS</td>" >> ./test_simulator_results.html
        echo "        <td>$NB_FAILURES</td>" >> ./test_simulator_results.html
        echo "        <td>$NB_SKIPPED</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
    done

    echo "   </table>" >> ./test_simulator_results.html

    echo "   <h4>Details</h4>" >> ./test_simulator_results.html
    for XML_FILE in $XML_TESTS
    do
        echo "   <table border = \"1\">" >> ./test_simulator_results.html
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
        echo "        <th>Test Name</th>" >> ./test_simulator_results.html
        echo "        <th>Description</th>" >> ./test_simulator_results.html
        echo "        <th>Result</th>" >> ./test_simulator_results.html
        echo "        <th>Time</th>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        TESTCASES_LIST=`sed -e "s# #@#g" $XML_FILE | grep testcase`
        for TESTCASE in $TESTCASES_LIST
        do
            echo "      <tr>" >> ./test_simulator_results.html
            NAME=`echo $TESTCASE | sed -e "s#^.*name='##" -e "s#'@description=.*##" | sed -e "s#@# #g"`
            echo "          <td>$NAME</td>" >> ./test_simulator_results.html
            DESC=`echo $TESTCASE | sed -e "s#^.*description='##" -e "s#'@Run_result=.*##" | sed -e "s#@# #g"`
            echo "          <td>$DESC</td>" >> ./test_simulator_results.html
            RESULT=`echo $TESTCASE | sed -e "s#^.*RESULT='##" -e "s#'.*##" | sed -e "s#@# #g"`
            if [[ $RESULT =~ .*PASS.* ]]
            then
                echo "          <td bgcolor = \"green\" >$RESULT</td>" >> ./test_simulator_results.html
            else
                echo "          <td bgcolor = \"red\" >$RESULT</td>" >> ./test_simulator_results.html
            fi
            TIME=`echo $TESTCASE | sed -e "s#^.*time='##" -e "s#'@RESULT=.*##" | sed -e "s#@# #g"`
            echo "          <td>$TIME</td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
        done
        echo "   </table>" >> ./test_simulator_results.html
    done
fi

echo "</body>" >> ./test_simulator_results.html
echo "</html>" >> ./test_simulator_results.html

exit 0
