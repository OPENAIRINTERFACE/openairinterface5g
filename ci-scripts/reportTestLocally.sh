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

function report_test_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool report-test [OPTIONS]"
    echo ""
    command_options_usage
}

function analyzePingFiles {
    for PING_CASE in $PING_LOGS
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
}

function analyzeIperfFiles {
    for IPERF_CASE in $IPERF_TESTS
    do
        echo "      <tr>" >> ./test_simulator_results.html
        NAME=`echo $IPERF_CASE | sed -e "s#$ARCHIVES_LOC/##"`
        echo "        <td>$NAME</td>" >> ./test_simulator_results.html
        CMD=`egrep "COMMAND IS" $IPERF_CASE | sed -e "s#COMMAND IS: ##"`
        echo "        <td>$CMD</td>" >> ./test_simulator_results.html
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
        FILE_COMPLETE=`egrep -c "Server Report" $IPERF_CASE`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            SERVER_FILE=`echo $IPERF_CASE | sed -e "s#client#server#"`
            FLOAT_EFF_BITRATE=`grep --color=never sec $SERVER_FILE | sed -e "s#^.*Bytes *##" -e "s#sec *.*#sec#" | awk 'BEGIN{s=0;n=0}{n++;if ($2 ~/Mbits/){a = $1 * 1000000};if ($2 ~/Kbits/){a = $1 * 1000};s=s+a}END{br=s/n; printf "%.0f", br}'`
            EFFECTIVE_BITRATE=`grep --color=never sec $SERVER_FILE | sed -e "s#^.*Bytes *##" -e "s#sec *.*#sec#" | awk 'BEGIN{s=0;n=0}{n++;if ($2 ~/Mbits/){a = $1 * 1000000};if ($2 ~/Kbits/){a = $1 * 1000};s=s+a}END{br=s/n; if(br>1000000){printf "%.2f MBits/sec", br/1000000}else{printf "%.2f KBits/sec", br/1000}}'`
            PERF=`echo "100 * $FLOAT_EFF_BITRATE / $FLOAT_REQ_BITRATE" | bc -l | awk '{printf "%.2f", $0}'`
            PERF_INT=`echo "100 * $FLOAT_EFF_BITRATE / $FLOAT_REQ_BITRATE" | bc -l | awk '{printf "%.0f", $0}'`
            JITTER=`grep --color=never sec $SERVER_FILE | sed -e "s#^.*/sec *##" -e "s# *ms.*##" | awk 'BEGIN{s=0;n=0}{n++;s+=$1}END{jitter=s/n; printf "%.3f ms", jitter}'`
            PACKETLOSS_NOSIGN=`grep --color=never sec $SERVER_FILE | sed -e "s#^.*(##" -e "s#%.*##" | awk 'BEGIN{s=0;n=0}{n++;s+=$1}END{per=s/n; printf "%.1f", per}'`
            PACKETLOSS=`echo "${PACKETLOSS_NOSIGN}%"`
            if [[ $PERF_INT -lt 80 ]]
            then
                echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            else
                echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
            fi
        else
            EFFECTIVE_BITRATE=`tail -n3 $IPERF_CASE | egrep "bits/sec" | sed -e "s#^.*Bytes *##" -e "s#sec.*#sec#"`
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
            if [[ $PERF_INT -lt 70 ]]
            then
                echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
            else
                echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
            fi
            EFFECTIVE_BITRATE=`tail -n3 $IPERF_CASE | egrep "bits/sec" | sed -e "s#^.*Bytes *##" -e "s#sec.*#sec#"`
            JITTER=`tail -n3 $IPERF_CASE | egrep "bits/sec" | sed -e "s#^.*bits/sec *##" -e "s#ms.*#ms#"`
            PACKETLOSS=`tail -n3 $IPERF_CASE | egrep "bits/sec" | sed -e "s#^.*(##" -e "s#).*##"`
        fi
        echo "        <td>" >> ./test_simulator_results.html
        echo "            <pre>" >> ./test_simulator_results.html
        echo "Bitrate      : $EFFECTIVE_BITRATE" >> ./test_simulator_results.html
        echo "Bitrate Perf : $PERF %" >> ./test_simulator_results.html
        echo "Jitter       : $JITTER" >> ./test_simulator_results.html
        echo "Packet Loss  : $PACKETLOSS" >> ./test_simulator_results.html
        echo "            </pre>" >> ./test_simulator_results.html
        echo "        </td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
    done
}

function report_test {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"

    echo "JENKINS_WKSP        = $JENKINS_WKSP"

    cd ${JENKINS_WKSP}

    echo "<!DOCTYPE html>" > ./test_simulator_results.html
    echo "<html class=\"no-js\" lang=\"en-US\">" >> ./test_simulator_results.html
    echo "<head>" >> ./test_simulator_results.html
    echo "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" >> ./test_simulator_results.html
    echo "  <link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\">" >> ./test_simulator_results.html
    echo "  <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>" >> ./test_simulator_results.html
    echo "  <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\"></script>" >> ./test_simulator_results.html
    echo "  <title>Simulator Results for $JOB_NAME job build #$BUILD_ID</title>" >> ./test_simulator_results.html
    echo "  <base href = \"http://www.openairinterface.org/\" />" >> ./test_simulator_results.html
    echo "</head>" >> ./test_simulator_results.html
    echo "<body><div class=\"container\">" >> ./test_simulator_results.html
    echo "  <br>" >> ./test_simulator_results.html
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
    echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-time\"></span> Build Start Time (UTC)</td>" >> ./test_simulator_results.html
    echo "        <td>TEMPLATE_BUILD_TIME</td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-cloud-upload\"></span> GIT Repository</td>" >> ./test_simulator_results.html
    echo "        <td><a href=\"$GIT_URL\">$GIT_URL</a></td>" >> ./test_simulator_results.html
    echo "      </tr>" >> ./test_simulator_results.html
    echo "      <tr>" >> ./test_simulator_results.html
    echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-wrench\"></span> Job Trigger</td>" >> ./test_simulator_results.html
    if [ $PU_TRIG -eq 1 ]; then echo "        <td>Push Event</td>" >> ./test_simulator_results.html; fi
    if [ $MR_TRIG -eq 1 ]; then echo "        <td>Merge-Request</td>" >> ./test_simulator_results.html; fi
    echo "      </tr>" >> ./test_simulator_results.html
    if [ $PU_TRIG -eq 1 ]
    then
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-tree-deciduous\"></span> Branch</td>" >> ./test_simulator_results.html
        echo "        <td>$SOURCE_BRANCH</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-tag\"></span> Commit ID</td>" >> ./test_simulator_results.html
        echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        if [ -e .git/CI_COMMIT_MSG ]
        then
            echo "      <tr>" >> ./test_simulator_results.html
            echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-comment\"></span> Commit Message</td>" >> ./test_simulator_results.html
            MSG=`cat .git/CI_COMMIT_MSG`
            echo "        <td>$MSG</td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
        fi
    fi
    if [ $MR_TRIG -eq 1 ]
    then
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-log-out\"></span> Source Branch</td>" >> ./test_simulator_results.html
        echo "        <td>$SOURCE_BRANCH</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-tag\"></span> Source Commit ID</td>" >> ./test_simulator_results.html
        echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        if [ -e .git/CI_COMMIT_MSG ]
        then
            echo "      <tr>" >> ./test_simulator_results.html
            echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-comment\"></span> Commit Message</td>" >> ./test_simulator_results.html
            MSG=`cat .git/CI_COMMIT_MSG`
            echo "        <td>$MSG</td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
        fi
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-log-in\"></span> Target Branch</td>" >> ./test_simulator_results.html
        echo "        <td>$TARGET_BRANCH</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
        echo "      <tr>" >> ./test_simulator_results.html
        echo "        <td bgcolor = \"lightcyan\" > <span class=\"glyphicon glyphicon-tag\"></span> Target Commit ID</td>" >> ./test_simulator_results.html
        echo "        <td>$TARGET_COMMIT_ID</td>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html
    fi
    echo "   </table>" >> ./test_simulator_results.html
    echo "   <h2>Test Summary</h2>" >> ./test_simulator_results.html

    ARCHIVES_LOC=archives/basic_sim/test
    if [ -d $ARCHIVES_LOC ]
    then
        echo "   <h3>4G LTE Basic Simulator Check</h3>" >> ./test_simulator_results.html

        if [ -f $ARCHIVES_LOC/test_final_status.log ]
        then
            if [ `grep -c TEST_OK $ARCHIVES_LOC/test_final_status.log` -eq 1 ]
            then
                echo "   <div class=\"alert alert-success\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was SUCCESSFUL <span class=\"glyphicon glyphicon-ok-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            else
                echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was a FAILURE! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            fi
        else
            echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
            echo "      <strong>COULD NOT DETERMINE TEST FINAL STATUS! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
            echo "   </div>" >> ./test_simulator_results.html
        fi

        echo "   <button data-toggle=\"collapse\" data-target=\"#oai-basic-sim-test-details\">More details on Basic Simulator test results</button>" >> ./test_simulator_results.html
        echo "   <div id=\"oai-basic-sim-test-details\" class=\"collapse\">" >> ./test_simulator_results.html
        echo "   <table border = \"1\">" >> ./test_simulator_results.html
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
        echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
        echo "        <th>Command</th>" >> ./test_simulator_results.html
        echo "        <th>Status</th>" >> ./test_simulator_results.html
        echo "        <th>Statistics</th>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html

        TRANS_MODES=("fdd" "tdd")
        BW_CASES=(05 10 20)
        for TMODE in ${TRANS_MODES[@]}
        do
            echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
            if [[ $TMODE =~ .*fdd.* ]]
            then
                echo "          <td align = \"center\" colspan = 4 >Test in FDD</td>" >> ./test_simulator_results.html
            else
                echo "          <td align = \"center\" colspan = 4 >Test in TDD</td>" >> ./test_simulator_results.html
            fi
            echo "      </tr>" >> ./test_simulator_results.html
            for BW in ${BW_CASES[@]}
            do
                ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}MHz_enb.log
                UE_LOG=`echo $ENB_LOG | sed -e "s#enb#ue#"`
                if [ -f $ENB_LOG ] && [ -f $UE_LOG ]
                then
                    NAME_ENB=`echo $ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>N/A</td>" >> ./test_simulator_results.html
                    NB_ENB_GOT_SYNC=`egrep -c "got sync" $ENB_LOG`
                    NB_UE_GOT_SYNC=`egrep -c "got sync" $UE_LOG`
                    NB_ENB_SYNCED_WITH_UE=`egrep -c "got UE capabilities for UE" $ENB_LOG`
                    if [ $NB_ENB_GOT_SYNC -gt 0 ] && [ $NB_UE_GOT_SYNC -eq 2 ] && [ $NB_ENB_SYNCED_WITH_UE -eq 1 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi
                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $NB_ENB_GOT_SYNC -gt 0 ]
                    then
                        echo "<font color = \"blue\">- eNB --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- eNB NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_UE_GOT_SYNC -eq 2 ]
                    then
                        echo "<font color = \"blue\">- UE --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- UE NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_ENB_SYNCED_WITH_UE -eq 1 ]
                    then
                        echo "<font color = \"blue\">- UE attached to eNB</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- UE NEVER attached to eNB</b></font>" >> ./test_simulator_results.html
                    fi
                    NB_SEGFAULT_ENB=`egrep -i -c "Segmentation Fault" $ENB_LOG`
                    if [ $NB_SEGFAULT_ENB -ne 0 ]
                    then
                        echo "<font color = \"red\"><b>- eNB --> Segmentation Fault</b></font>" >> ./test_simulator_results.html
                    fi
                    NB_SEGFAULT_UE=`egrep -i -c "Segmentation Fault" $UE_LOG`
                    if [ $NB_SEGFAULT_UE -ne 0 ]
                    then
                        echo "<font color = \"red\"><b>- UE --> Segmentation Fault</b></font>" >> ./test_simulator_results.html
                    fi
                    NB_ASSERTION_ENB=`egrep -i -c "Assertion" $ENB_LOG`
                    if [ $NB_ASSERTION_ENB -ne 0 ]
                    then
                        echo "<font color = \"red\"><b>- eNB --> Assertion</b></font>" >> ./test_simulator_results.html
                        awk 'BEGIN{assertion=10}{if(assertion < 3){print "    " $0; assertion++};if ($0 ~/Assertion/){print "    " $0;assertion=1}}END{}' $ENB_LOG >> ./test_simulator_results.html
                    fi
                    NB_ASSERTION_UE=`egrep -i -c "Assertion" $UE_LOG`
                    if [ $NB_ASSERTION_UE -ne 0 ]
                    then
                        echo "<font color = \"red\"><b>- eNB --> Assertion</b></font>" >> ./test_simulator_results.html
                        awk 'BEGIN{assertion=10}{if(assertion < 3){print "    " $0; assertion++};if ($0 ~/Assertion/){print "    " $0;assertion=1}}END{}' $UE_LOG >> ./test_simulator_results.html
                    fi
                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi
                PING_LOGS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_ping_ue.txt 2> /dev/null`
                analyzePingFiles
        
                IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}*iperf*client*txt 2> /dev/null`
                analyzeIperfFiles
            done
        done

        echo "   </table>" >> ./test_simulator_results.html
        echo "   </div>" >> ./test_simulator_results.html
    fi

    if [ -e $JENKINS_WKSP/flexran/flexran_build_complete.txt ]
    then
        echo "   <h3>4G LTE Basic Simulator + FlexRan Controller Check</h3>" >> ./test_simulator_results.html
        echo "   <div class=\"alert alert-success\">" >> ./test_simulator_results.html
        echo "      <strong>TEST was SUCCESSFUL <span class=\"glyphicon glyphicon-ok-circle\"></span></strong>" >> ./test_simulator_results.html
        echo "   </div>" >> ./test_simulator_results.html
        echo "   <button data-toggle=\"collapse\" data-target=\"#oai-flexran-test-details\">More details on Basic Simulator + Fleran Controller test results</button>" >> ./test_simulator_results.html
        echo "   <div id=\"oai-flexran-test-details\" class=\"collapse\">" >> ./test_simulator_results.html
        echo "   <table border = \"1\">" >> ./test_simulator_results.html
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
        echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
        echo "        <th>JSON Query Response</th>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html

        FLEXRAN_QUERIES=`ls $ARCHIVES_LOC/flexran_ctl_query_*log`
        for QUERY in $FLEXRAN_QUERIES
        do
            echo "      <tr>" >> ./test_simulator_results.html
            NAME=`echo $QUERY | sed -e "s#$ARCHIVES_LOC/##"`
            echo "        <td>$NAME</td>" >> ./test_simulator_results.html
            echo "        <td><pre><code>" >> ./test_simulator_results.html
            egrep -v "LOG_NAME|\-\-\-\-\-" $QUERY >> ./test_simulator_results.html
            echo "        </code></pre></td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
        done
        echo "   </table>" >> ./test_simulator_results.html
        echo "   </div>" >> ./test_simulator_results.html
    fi

    ARCHIVES_LOC=archives/rf_sim/test
    if [ -d $ARCHIVES_LOC ]
    then
        echo "   <h3>4G LTE RF Simulator Check</h3>" >> ./test_simulator_results.html

        if [ -f $ARCHIVES_LOC/test_final_status.log ]
        then
            if [ `egrep -c "LTE: TEST_OK" $ARCHIVES_LOC/test_final_status.log` -eq 1 ]
            then
                echo "   <div class=\"alert alert-success\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was SUCCESSFUL <span class=\"glyphicon glyphicon-ok-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            else
                echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was a FAILURE! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            fi
        else
            echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
            echo "      <strong>COULD NOT DETERMINE TEST FINAL STATUS! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
            echo "   </div>" >> ./test_simulator_results.html
        fi

        echo "   <button data-toggle=\"collapse\" data-target=\"#oai-lte-rf-sim-test-details\">More details on 4G LTE RF Simulator test results</button>" >> ./test_simulator_results.html
        echo "   <div id=\"oai-lte-rf-sim-test-details\" class=\"collapse\">" >> ./test_simulator_results.html
        echo "   <table border = \"1\">" >> ./test_simulator_results.html
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
        echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
        echo "        <th>Command</th>" >> ./test_simulator_results.html
        echo "        <th>Status</th>" >> ./test_simulator_results.html
        echo "        <th>Statistics</th>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html

        EPC_CONFIGS=("wS1 noS1")
        TRANS_MODES=("fdd")
        BW_CASES=(05 10)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          for TMODE in ${TRANS_MODES[@]}
          do
            for BW in ${BW_CASES[@]}
            do
                echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
                if [[ $CN_CONFIG =~ .*wS1.* ]]
                then
                    echo "          <td align = \"center\" colspan = 4 >Test with EPC (aka withS1): ${TMODE} -- ${BW}MHz </td>" >> ./test_simulator_results.html
                else
                    echo "          <td align = \"center\" colspan = 4 >Test without EPC (aka noS1): ${TMODE} -- ${BW}MHz </td>" >> ./test_simulator_results.html
                fi
                echo "      </tr>" >> ./test_simulator_results.html
                ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_enb.log
                UE_LOG=`echo $ENB_LOG | sed -e "s#enb#ue#"`
                if [ -f $ENB_LOG ] && [ -f $UE_LOG ]
                then
                    NAME_ENB=`echo $ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>N/A</td>" >> ./test_simulator_results.html
                    NB_ENB_GOT_SYNC=`egrep -c "got sync" $ENB_LOG`
                    if [[ $CN_CONFIG =~ .*noS1.* ]]
                    then
                        NB_ENB_TUNNEL_UP=`egrep -c "Interface oaitun_enb1 successfully configured" $ENB_LOG`
                    fi
                    NB_UE_GOT_SYNC=`egrep -c "rfsimulator: Success" $UE_LOG`
                    NB_ENB_SYNCED_WITH_UE=`egrep -c "Generating RRCConnectionReconfigurationComplete" $UE_LOG`
                    if [[ $CN_CONFIG =~ .*noS1.* ]]
                    then
                        NB_UE_TUNNEL_UP=`egrep -c "Interface oaitun_ue1 successfully configured" $UE_LOG`
                    else
                        NB_UE_TUNNEL_UP=`egrep -c "ip link set oaitun_ue1 up" $UE_LOG`
                    fi
                    if [ $NB_ENB_GOT_SYNC -gt 0 ] && [ $NB_UE_GOT_SYNC -gt 0 ] && [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi
                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $NB_ENB_GOT_SYNC -gt 0 ]
                    then
                        echo "<font color = \"blue\">- eNB --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- eNB NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [[ $CN_CONFIG =~ .*noS1.* ]]
                    then
                        if [ $NB_ENB_TUNNEL_UP -gt 0 ]
                        then
                            echo "<font color = \"blue\">- eNB mounted oaitun_enb1 interface</font>" >> ./test_simulator_results.html
                        else
                            echo "<font color = \"red\"><b>- eNB NEVER mounted oaitun_enb1 interface</b></font>" >> ./test_simulator_results.html
                        fi
                    fi
                    if [ $NB_UE_GOT_SYNC -gt 0 ]
                    then
                        echo "<font color = \"blue\">- LTE UE --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- LTE UE NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                    then
                        echo "<font color = \"blue\">- LTE UE attached to eNB</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- LTE UE NEVER attached to eNB</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_UE_TUNNEL_UP -gt 0 ]
                    then
                        echo "<font color = \"blue\">- LTE UE mounted oaitun_ue1 interface</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- LTE UE NEVER mounted oaitun_ue1 interface</b></font>" >> ./test_simulator_results.html
                    fi
                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi
                PING_LOGS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_ping*.log 2> /dev/null`
                analyzePingFiles

                IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl*client*txt | grep -v mbms 2> /dev/null`
                analyzeIperfFiles

                IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_ul*client*txt | grep -v mbms 2> /dev/null`
                analyzeIperfFiles
            done
          done
        done

        # MBMS Case
        CN_CONFIG="noS1"
        TMODE="fdd"
        BW_CASES=(05)
        for BW in ${BW_CASES[@]}
        do
            echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
            echo "          <td align = \"center\" colspan = 4 >Test MBMS without EPC (aka noS1): ${TMODE} -- ${BW}MHz </td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
            ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_enb_mbms.log
            UE_LOG=`echo $ENB_LOG | sed -e "s#enb#ue#"`
            if [ -f $ENB_LOG ] && [ -f $UE_LOG ]
            then
                NAME_ENB=`echo $ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                NAME_UE=`echo $UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                echo "      <tr>" >> ./test_simulator_results.html
                echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                echo "        <td>N/A</td>" >> ./test_simulator_results.html
                NB_ENB_GOT_SYNC=`egrep -c "got sync" $ENB_LOG`
                NB_ENB_TUNNEL_UP=`egrep -c "Interface oaitun_enb1 successfully configured" $ENB_LOG`
                NB_ENB_MTUNNEL_UP=`egrep -c "Interface oaitun_enm1 successfully configured" $ENB_LOG`
                NB_UE_GOT_SYNC=`egrep -c "rfsimulator: Success" $UE_LOG`
                NB_ENB_SYNCED_WITH_UE=`egrep -c "Generating RRCConnectionReconfigurationComplete" $UE_LOG`
                NB_UE_TUNNEL_UP=`egrep -c "Interface oaitun_ue1 successfully configured" $UE_LOG`
                NB_UE_MTUNNEL_UP=`egrep -c "Interface oaitun_uem1 successfully configured" $UE_LOG`
                NB_UE_MBMS_PUSH_MSG=`egrep -c "TRIED TO PUSH MBMS DATA TO" $UE_LOG`
                if [ $NB_ENB_GOT_SYNC -gt 0 ] && [ $NB_UE_GOT_SYNC -gt 0 ] && [ $NB_ENB_SYNCED_WITH_UE -gt 0 ] && [ $NB_UE_MBMS_PUSH_MSG -gt 0 ]
                then
                    echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                else
                    echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                fi
                echo "        <td><pre>" >> ./test_simulator_results.html
                if [ $NB_ENB_GOT_SYNC -gt 0 ]
                then
                    echo "<font color = \"blue\">- eNB --> got sync</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- eNB NEVER got sync</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_ENB_TUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- eNB mounted oaitun_enb1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- eNB NEVER mounted oaitun_enb1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_ENB_MTUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- eNB mounted oaitun_enm1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- eNB NEVER mounted oaitun_enm1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_GOT_SYNC -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE --> got sync</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER got sync</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE attached to eNB</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER attached to eNB</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_TUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE mounted oaitun_ue1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER mounted oaitun_ue1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_MTUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE mounted oaitun_uem1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER mounted oaitun_uem1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_MBMS_PUSH_MSG -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE tried to push ${NB_UE_MBMS_PUSH_MSG} MBMS DATA</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER pushed MBMS DATA</b></font>" >> ./test_simulator_results.html
                fi
                echo "        </pre></td>" >> ./test_simulator_results.html
                echo "      </tr>" >> ./test_simulator_results.html
            fi
            #PING_LOGS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_ping*.log 2> /dev/null`
            #analyzePingFiles

            #IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl*client*txt | grep -v mbms 2> /dev/null`
            #analyzeIperfFiles

            #IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_ul*client*txt | grep -v mbms 2> /dev/null`
            #analyzeIperfFiles
        done

        # FeMBMS Case
        CN_CONFIG="noS1"
        TMODE="fdd"
        BW_CASES=(05)
        for BW in ${BW_CASES[@]}
        do
            echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
            echo "          <td align = \"center\" colspan = 4 >Test FeMBMS without EPC (aka noS1): ${TMODE} -- ${BW}MHz </td>" >> ./test_simulator_results.html
            echo "      </tr>" >> ./test_simulator_results.html
            ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_enb_fembms.log
            UE_LOG=`echo $ENB_LOG | sed -e "s#enb#ue#"`
            if [ -f $ENB_LOG ] && [ -f $UE_LOG ]
            then
                NAME_ENB=`echo $ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                NAME_UE=`echo $UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                echo "      <tr>" >> ./test_simulator_results.html
                echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                echo "        <td>N/A</td>" >> ./test_simulator_results.html
                #NB_ENB_GOT_SYNC=`egrep -c "got sync" $ENB_LOG`
                NB_ENB_TUNNEL_UP=`egrep -c "Interface oaitun_enb1 successfully configured" $ENB_LOG`
                NB_ENB_MTUNNEL_UP=`egrep -c "Interface oaitun_enm1 successfully configured" $ENB_LOG`
                #NB_UE_GOT_SYNC=`egrep -c "rfsimulator: Success" $UE_LOG`
                #NB_ENB_SYNCED_WITH_UE=`egrep -c "Generating RRCConnectionReconfigurationComplete" $UE_LOG`
                NB_UE_TUNNEL_UP=`egrep -c "Interface oaitun_ue1 successfully configured" $UE_LOG`
                NB_UE_MTUNNEL_UP=`egrep -c "Interface oaitun_uem1 successfully configured" $UE_LOG`
                NB_UE_MBMS_PUSH_MSG=`egrep -c "TRIED TO PUSH MBMS DATA TO" $UE_LOG`
                #if [ $NB_ENB_GOT_SYNC -gt 0 ] && [ $NB_UE_GOT_SYNC -gt 0 ] && [ $NB_ENB_SYNCED_WITH_UE -gt 0 ] && [ $NB_UE_MBMS_PUSH_MSG -gt 0 ]
                if  [ $NB_UE_MBMS_PUSH_MSG -gt 0 ]
                then
                    echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                else
                    echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                fi
                echo "        <td><pre>" >> ./test_simulator_results.html
                #if [ $NB_ENB_GOT_SYNC -gt 0 ]
                #then
                #    echo "<font color = \"blue\">- eNB --> got sync</font>" >> ./test_simulator_results.html
                #else
                #    echo "<font color = \"red\"><b>- eNB NEVER got sync</b></font>" >> ./test_simulator_results.html
                #fi
                if [ $NB_ENB_TUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- eNB mounted oaitun_enb1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- eNB NEVER mounted oaitun_enb1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_ENB_MTUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- eNB mounted oaitun_enm1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- eNB NEVER mounted oaitun_enm1 interface</b></font>" >> ./test_simulator_results.html
                fi
                #if [ $NB_UE_GOT_SYNC -gt 0 ]
                #then
                #    echo "<font color = \"blue\">- LTE UE --> got sync</font>" >> ./test_simulator_results.html
                #else
                #    echo "<font color = \"red\"><b>- LTE UE NEVER got sync</b></font>" >> ./test_simulator_results.html
                #fi
                #if [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                #then
                #    echo "<font color = \"blue\">- LTE UE attached to eNB</font>" >> ./test_simulator_results.html
                #else
                #    echo "<font color = \"red\"><b>- LTE UE NEVER attached to eNB</b></font>" >> ./test_simulator_results.html
                #fi
                if [ $NB_UE_TUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE mounted oaitun_ue1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER mounted oaitun_ue1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_MTUNNEL_UP -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE mounted oaitun_uem1 interface</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER mounted oaitun_uem1 interface</b></font>" >> ./test_simulator_results.html
                fi
                if [ $NB_UE_MBMS_PUSH_MSG -gt 0 ]
                then
                    echo "<font color = \"blue\">- LTE UE tried to push ${NB_UE_MBMS_PUSH_MSG} MBMS DATA</font>" >> ./test_simulator_results.html
                else
                    echo "<font color = \"red\"><b>- LTE UE NEVER pushed MBMS DATA</b></font>" >> ./test_simulator_results.html
                fi
                echo "        </pre></td>" >> ./test_simulator_results.html
                echo "      </tr>" >> ./test_simulator_results.html
            fi
            #PING_LOGS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_ping*.log 2> /dev/null`
            #analyzePingFiles

            #IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl*client*txt | grep -v mbms 2> /dev/null`
            #analyzeIperfFiles

            #IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_ul*client*txt | grep -v mbms 2> /dev/null`
            #analyzeIperfFiles
        done
        echo "   </table>" >> ./test_simulator_results.html
        echo "   </div>" >> ./test_simulator_results.html
    fi

    ARCHIVES_LOC=archives/rf5g_sim/test
    if [ -d $ARCHIVES_LOC ]
    then
        echo "   <h3>5G NR RF Simulator Check</h3>" >> ./test_simulator_results.html

        if [ -f $ARCHIVES_LOC/test_final_status.log ]
        then
            if [ `egrep -c "5G-NR: TEST_OK" $ARCHIVES_LOC/test_final_status.log` -eq 1 ]
            then
                echo "   <div class=\"alert alert-success\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was SUCCESSFUL <span class=\"glyphicon glyphicon-ok-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            else
                echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was a FAILURE! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            fi
        else
            echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
            echo "      <strong>COULD NOT DETERMINE TEST FINAL STATUS! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
            echo "   </div>" >> ./test_simulator_results.html
        fi

        echo "   <button data-toggle=\"collapse\" data-target=\"#oai-nr-rf-sim-test-details\">More details on 5G NR RF Simulator test results</button>" >> ./test_simulator_results.html
        echo "   <div id=\"oai-nr-rf-sim-test-details\" class=\"collapse\">" >> ./test_simulator_results.html
        echo "   <table border = \"1\">" >> ./test_simulator_results.html
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./test_simulator_results.html
        echo "        <th>Log File Name</th>" >> ./test_simulator_results.html
        echo "        <th>Command</th>" >> ./test_simulator_results.html
        echo "        <th>Status</th>" >> ./test_simulator_results.html
        echo "        <th>Statistics</th>" >> ./test_simulator_results.html
        echo "      </tr>" >> ./test_simulator_results.html

        #SA
        EPC_CONFIGS=("noS1")
        TRANS_MODES=("tdd")
        FR_MODE=("SA")
        BW_CASES=(106)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          for TMODE in ${TRANS_MODES[@]}
          do
            for BW in ${BW_CASES[@]}
            do
                echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
                if [[ $CN_CONFIG =~ .*wS1.* ]]
                then
                    echo "          <td align = \"center\" colspan = 4 >Test with CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                else
                    echo "          <td align = \"center\" colspan = 4 >Test without CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                fi
                echo "      </tr>" >> ./test_simulator_results.html


                #SA test (--sa option)

                SA_ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_gnb_sa_test.log
                SA_UE_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_ue_sa_test.log
                if [ -f $RA_ENB_LOG ] && [ -f $RA_UE_LOG ]
                then
                    #get rid of full path
                    NAME_ENB=`echo $SA_ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $SA_UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>Check if SA proc succeeded</td>" >> ./test_simulator_results.html


                    RRC_CHECK=`egrep -c "Received rrcSetupComplete" $SA_ENB_LOG`
                    CBRA_CHECK=`egrep -c "Received Ack of RA-Msg4\. CBRA procedure succeeded" $SA_ENB_LOG`
                    SIB1_CHECK=`egrep -c "SIB1 decoded" $SA_UE_LOG`


                    if [ $RRC_CHECK -gt 0 ] && [ $CBRA_CHECK -gt 0 ] && [ $SIB1_CHECK -gt 0 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi

                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $RRC_CHECK -gt 0 ]
                    then
                        echo "<font color = \"blue\">- Received rrcSetupComplete OK</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- Received rrcSetupComplete KO</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $CBRA_CHECK -gt 0 ]
                    then
                        echo "<font color = \"blue\">- CBRA procedure succeeded OK</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- CBRA procedure succeeded KO</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $SIB1_CHECK -gt 0 ]
                    then
                        echo "<font color = \"blue\">- SIB1 decoded OK</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- SIB1 decoded KO</b></font>" >> ./test_simulator_results.html
                    fi

                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi

            done
          done
        done

        
        EPC_CONFIGS=("noS1")
        TRANS_MODES=("tdd")
        FR_MODE=("FR2")
        BW_CASES=(32)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          for TMODE in ${TRANS_MODES[@]}
          do
            for BW in ${BW_CASES[@]}
            do
                echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
                if [[ $CN_CONFIG =~ .*wS1.* ]]
                then
                    echo "          <td align = \"center\" colspan = 4 >Test with CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                else
                    echo "          <td align = \"center\" colspan = 4 >Test without CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                fi
                echo "      </tr>" >> ./test_simulator_results.html


                #FR2 RA test (--do-ra option)

                #build log files names tdd_${PRB}prb_${CN_CONFIG}_gnb_ra_fr2_test.log

                RA_ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_gnb_ra_fr2_test.log
                RA_UE_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_ue_ra_fr2_test.log
                if [ -f $RA_ENB_LOG ] && [ -f $RA_UE_LOG ]
                then
                    #get rid of full path
                    NAME_ENB=`echo $RA_ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $RA_UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>Check if FR2 RA proc succeeded</td>" >> ./test_simulator_results.html

                    #gNB RA check
                    GNB_RECEIVED=`egrep -c "\[RAPROC\] PUSCH with TC_RNTI (.+) received correctly" $RA_ENB_LOG`
                    #UE RA check
                    UE_RA_PROC_OK=`egrep -c "\[RAPROC\] RA procedure succeeded" $RA_UE_LOG`


                    if [ $GNB_RECEIVED -gt 0 ] && [ $UE_RA_PROC_OK -gt 0 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi

                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $GNB_RECEIVED -gt 0 ]
                    then
                        echo "<font color = \"blue\">- gNB --> RA received</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- gNB RA NOT RECEIVED</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $UE_RA_PROC_OK -gt 0 ]
                    then
                        echo "<font color = \"blue\">- NR UE  --> RA procedure succeded</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- NR UE RA procedure failed</b></font>" >> ./test_simulator_results.html
                    fi
                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi

            done
          done
        done
    

        EPC_CONFIGS=("noS1")
        FR_MODE=("FR1")
        TRANS_MODES=("fdd" "tdd")
        BW_CASES=(106)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          for TMODE in ${TRANS_MODES[@]}
          do
            for BW in ${BW_CASES[@]}
            do
                echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
                if [[ $CN_CONFIG =~ .*wS1.* ]]
                then
                    echo "          <td align = \"center\" colspan = 4 >Test with CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                else
                    echo "          <td align = \"center\" colspan = 4 >Test without CN5G : ${TMODE} -- ${BW}PRB -- ${FR_MODE}</td>" >> ./test_simulator_results.html
                fi
                echo "      </tr>" >> ./test_simulator_results.html


                #FR1 RA test (--do-ra option)

                #build log files names
                RA_ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_gnb_ra_test.log
                RA_UE_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_ue_ra_test.log
                if [ -f $RA_ENB_LOG ] && [ -f $RA_UE_LOG ]
                then
                    #get rid of full path
                    NAME_ENB=`echo $RA_ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $RA_UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>Check if FR1 RA proc succeeded</td>" >> ./test_simulator_results.html

                    #gNB RA check
                    GNB_RECEIVED=`egrep -c "\[RAPROC\] PUSCH with TC_RNTI (.+) received correctly" $RA_ENB_LOG`
                    #UE RA check
                    UE_RA_PROC_OK=`egrep -c "\[RAPROC\] RA procedure succeeded" $RA_UE_LOG`


                    if [ $GNB_RECEIVED -gt 0 ] && [ $UE_RA_PROC_OK -gt 0 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi

                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $GNB_RECEIVED -gt 0 ]
                    then
                        echo "<font color = \"blue\">- gNB --> RA received</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- gNB RA NOT RECEIVED</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $UE_RA_PROC_OK -gt 0 ]
                    then
                        echo "<font color = \"blue\">- NR UE  --> RA procedure succeded</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- NR UE RA procedure failed</b></font>" >> ./test_simulator_results.html
                    fi
                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi

                #SYNC test
                ENB_LOG=$ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_gnb.log
                UE_LOG=`echo $ENB_LOG | sed -e "s#gnb#ue#"`
                if [ -f $ENB_LOG ] && [ -f $UE_LOG ]
                then
                    NAME_ENB=`echo $ENB_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    NAME_UE=`echo $UE_LOG | sed -e "s#$ARCHIVES_LOC/##"`
                    echo "      <tr>" >> ./test_simulator_results.html
                    echo "        <td>$NAME_ENB --- $NAME_UE</td>" >> ./test_simulator_results.html
                    echo "        <td>N/A</td>" >> ./test_simulator_results.html
                    NB_ENB_GOT_SYNC=`egrep -c "got sync" $ENB_LOG`
                    NB_ENB_TUNNEL_UP=`egrep -c "Interface oaitun_enb1 successfully configured" $ENB_LOG`
                    NB_UE_GOT_SYNC=`egrep -c "rfsimulator: Success" $UE_LOG`
                    NB_ENB_SYNCED_WITH_UE=`egrep -c "Initial sync: starting PBCH detection" $UE_LOG`
                    NB_UE_TUNNEL_UP=`egrep -c "Interface oaitun_ue1 successfully configured" $UE_LOG`
                    if [ $NB_ENB_GOT_SYNC -gt 0 ] && [ $NB_UE_GOT_SYNC -gt 0 ] && [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                    then
                        echo "        <td bgcolor = \"green\" >OK</td>" >> ./test_simulator_results.html
                    else
                        echo "        <td bgcolor = \"red\" >KO</td>" >> ./test_simulator_results.html
                    fi
                    echo "        <td><pre>" >> ./test_simulator_results.html
                    if [ $NB_ENB_GOT_SYNC -gt 0 ]
                    then
                        echo "<font color = \"blue\">- gNB --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- gNB NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_ENB_TUNNEL_UP -gt 0 ]
                    then
                        echo "<font color = \"blue\">- gNB mounted oaitun_enb1 interface</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- gNB NEVER mounted oaitun_enb1 interface</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_UE_GOT_SYNC -gt 0 ]
                    then
                        echo "<font color = \"blue\">- NR UE --> got sync</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- NR UE NEVER got sync</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_ENB_SYNCED_WITH_UE -gt 0 ]
                    then
                        echo "<font color = \"blue\">- NR UE attached to gNB</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- NR UE NEVER attached to eNB</b></font>" >> ./test_simulator_results.html
                    fi
                    if [ $NB_UE_TUNNEL_UP -gt 0 ]
                    then
                        echo "<font color = \"blue\">- NR UE mounted oaitun_ue1 interface</font>" >> ./test_simulator_results.html
                    else
                        echo "<font color = \"red\"><b>- NR UE NEVER mounted oaitun_ue1 interface</b></font>" >> ./test_simulator_results.html
                    fi
                    echo "        </pre></td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                fi
                PING_LOGS=`ls $ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_ping*.log 2> /dev/null`
                analyzePingFiles

                IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_iperf_dl*client*txt 2> /dev/null`
                analyzeIperfFiles

                IPERF_TESTS=`ls $ARCHIVES_LOC/${TMODE}_${BW}prb_${CN_CONFIG}_iperf_ul*client*txt 2> /dev/null`
                analyzeIperfFiles
            done
          done
        done

        echo "   </table>" >> ./test_simulator_results.html
        echo "   </div>" >> ./test_simulator_results.html
    fi

    ARCHIVES_LOC=archives/phy_sim/test
    if [ -d $ARCHIVES_LOC ]
    then
        echo "   <h3>Physical Simulators Check</h3>" >> ./test_simulator_results.html

        if [ -f $ARCHIVES_LOC/test_final_status.log ]
        then
            if [ `grep -c TEST_OK $ARCHIVES_LOC/test_final_status.log` -eq 1 ]
            then
                echo "   <div class=\"alert alert-success\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was SUCCESSFUL <span class=\"glyphicon glyphicon-ok-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            else
                echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
                echo "      <strong>TEST was a FAILURE! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
                echo "   </div>" >> ./test_simulator_results.html
            fi
        else
            echo "   <div class=\"alert alert-danger\">" >> ./test_simulator_results.html
            echo "      <strong>COULD NOT DETERMINE TEST FINAL STATUS! <span class=\"glyphicon glyphicon-ban-circle\"></span></strong>" >> ./test_simulator_results.html
            echo "   </div>" >> ./test_simulator_results.html
        fi

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
        echo "   <br>" >> ./test_simulator_results.html

        echo "   <button data-toggle=\"collapse\" data-target=\"#oai-phy-sim-test-details\">More details on Physical Simulators test results</button>" >> ./test_simulator_results.html
        echo "   <div id=\"oai-phy-sim-test-details\" class=\"collapse\">" >> ./test_simulator_results.html

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
            PREV_SECTION=0
            PREV_TIME_IN_SECS=0
            TESTCASES_LIST=`sed -e "s# #@#g" $XML_FILE | grep testcase`
            for TESTCASE in $TESTCASES_LIST
            do
                NAME=`echo $TESTCASE | sed -e "s#^.*name='##" -e "s#'@description=.*##" | sed -e "s#@# #g"`
                SECTION=`echo $NAME | sed -e "s#\..*##"`
                if [ $SECTION != $PREV_SECTION ]
                then
                    echo "      <tr bgcolor = \"#8FBC8F\" >" >> ./test_simulator_results.html
                    echo "          <td align = \"center\" colspan = 4 >\"$SECTION\" series</td>" >> ./test_simulator_results.html
                    echo "      </tr>" >> ./test_simulator_results.html
                    PREV_SECTION=$SECTION
                    PREV_TIME_IN_SECS=0
                fi
                DESC=`echo $TESTCASE | sed -e "s#^.*description='##" -e "s#'@Run_result=.*##" | sed -e "s#@# #g"`
                RESULT=`echo $TESTCASE | sed -e "s#^.*RESULT='##" -e "s#'.*##" | sed -e "s#@# #g"`
                TIME_IN_SECS=`echo $TESTCASE | sed -e "s#^.*time='##" -e "s#'@RESULT=.*##" | sed -e "s#@# #g" -e "s# s.*##"`
                TIME=`echo "$TIME_IN_SECS - $PREV_TIME_IN_SECS" | bc -l | awk '{printf "%.2f s", $0}'`
                PREV_TIME_IN_SECS=$TIME_IN_SECS
                echo "      <tr>" >> ./test_simulator_results.html
                echo "          <td>$NAME</td>" >> ./test_simulator_results.html
                echo "          <td>$DESC</td>" >> ./test_simulator_results.html
                if [[ $RESULT =~ .*PASS.* ]]
                then
                    echo "          <td bgcolor = \"green\" >$RESULT</td>" >> ./test_simulator_results.html
                else
                    SPLITTED_LINE=`echo -e $TESTCASE | sed -e "s#@#\n#g"`
                    NB_RUNS=`echo -e "${SPLITTED_LINE}" | grep -v Run_result | egrep -c "Run_"`
                    NB_FAILS=`echo -e "${SPLITTED_LINE}" | grep -v Run_result | egrep -c "=FAIL"`
                    echo "          <td bgcolor = \"red\" >${RESULT} (${NB_FAILS}/${NB_RUNS})</td>" >> ./test_simulator_results.html
                fi
                echo "          <td>$TIME</td>" >> ./test_simulator_results.html
                echo "      </tr>" >> ./test_simulator_results.html
            done
            echo "   </table>" >> ./test_simulator_results.html
        done
    fi

    echo "   </div>" >> ./test_simulator_results.html
    echo "   <p></p>" >> ./test_simulator_results.html
    echo "   <div class=\"well well-lg\">End of Test Report -- Copyright <span class=\"glyphicon glyphicon-copyright-mark\"></span> 2018 <a href=\"http://www.openairinterface.org/\">OpenAirInterface</a>. All Rights Reserved.</div>" >> ./test_simulator_results.html
    echo "</div></body>" >> ./test_simulator_results.html
    echo "</html>" >> ./test_simulator_results.html
}
