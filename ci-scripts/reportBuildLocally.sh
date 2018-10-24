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
    echo "OAI Local Build Report script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo ""
    echo "    reportBuildLocally.sh [OPTIONS]"
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
    echo "OAI Local Build Report script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "    --trigger merge-request OR -mr"
    echo "    --trigger push          OR -pu"
    echo "    Specify trigger action of the Jenkins job. Either a merge-request event or a push event."
    echo ""
}

function details_table {
    echo "   <h4>$1</h4>" >> $3

    echo "   <table border = \"1\">" >> $3
    echo "      <tr bgcolor = \"#33CCFF\" >" >> $3
    echo "        <th>File</th>" >> $3
    echo "        <th>Line Number</th>" >> $3
    echo "        <th>Status</th>" >> $3
    echo "        <th>Message</th>" >> $3
    echo "      </tr>" >> $3

    LIST_MESSAGES=`egrep "error:|warning:" $2 | egrep -v "jobserver unavailable|Clock skew detected.|flexran.proto"`
    COMPLETE_MESSAGE="start"
    for MESSAGE in $LIST_MESSAGES
    do
        if [[ $MESSAGE == *"/home/ubuntu/tmp"* ]]
        then
            FILENAME=`echo $MESSAGE | sed -e "s#^/home/ubuntu/tmp/##" -e "s#^.*/tmp/CI-eNB/##" | awk -F ":" '{print $1}'`
            LINENB=`echo $MESSAGE | awk -F ":" '{print $2}'`
            if [ "$COMPLETE_MESSAGE" != "start" ]
            then
                COMPLETE_MESSAGE=`echo $COMPLETE_MESSAGE | sed -e "s#‘#'#g" -e "s#’#'#g"`
                echo "        <td>$COMPLETE_MESSAGE</td>" >> $3
                echo "      </tr>" >> $3
            fi
            echo "      <tr>" >> $3
            echo "        <td>$FILENAME</td>" >> $3
            echo "        <td>$LINENB</td>" >> $3
        else
            if [[ $MESSAGE == *"warning:"* ]] || [[ $MESSAGE == *"error:"* ]]
            then
                MSGTYPE=`echo $MESSAGE | sed -e "s#:##g"`
                echo "        <td>$MSGTYPE</td>" >> $3
                COMPLETE_MESSAGE=""
            else
                COMPLETE_MESSAGE=$COMPLETE_MESSAGE" "$MESSAGE
            fi
        fi
    done

    if [ "$COMPLETE_MESSAGE" != "start" ]
    then
        COMPLETE_MESSAGE=`echo $COMPLETE_MESSAGE | sed -e "s#‘#'#g" -e "s#’#'#g"`
        echo "        <td>$COMPLETE_MESSAGE</td>" >> $3
        echo "      </tr>" >> $3
    fi
    echo "   </table>" >> $3
}

function summary_table_header {
    echo "   <h3>$1</h3>" >> ./build_results.html
    echo "   <table border = \"1\">" >> ./build_results.html
    echo "      <tr bgcolor = \"#33CCFF\" >" >> ./build_results.html
    echo "        <th>Element</th>" >> ./build_results.html
    echo "        <th>Status</th>" >> ./build_results.html
    echo "        <th>Nb Errors</th>" >> ./build_results.html
    echo "        <th>Nb Warnings</th>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
}

function summary_table_row {
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >$1</th>" >> ./build_results.html
    if [ -f $2 ]
    then
        STATUS=`egrep -c "$3" $2`
        if [ $STATUS -eq 1 ]
        then
            echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
        else
            echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
        fi
        NB_ERRORS=`egrep -c "error:" $2`
        if [ $NB_ERRORS -eq 0 ]
        then
            echo "        <td bgcolor = \"green\" >$NB_ERRORS</th>" >> ./build_results.html
        else
            echo "        <td bgcolor = \"red\" >$NB_ERRORS</th>" >> ./build_results.html
        fi
        NB_WARNINGS=`egrep "warning:" $2 | egrep -v "jobserver unavailable|Clock skew detected.|flexran.proto" | egrep -c "warning:"`
        if [ $NB_WARNINGS -eq 0 ]
        then
            echo "        <td bgcolor = \"green\" >$NB_WARNINGS</th>" >> ./build_results.html
        else
            if [ $NB_WARNINGS -gt 20 ]
            then
                echo "        <td bgcolor = \"red\" >$NB_WARNINGS</th>" >> ./build_results.html
            else
                echo "        <td bgcolor = \"orange\" >$NB_WARNINGS</th>" >> ./build_results.html
            fi
        fi
        if [ $NB_ERRORS -ne 0 ] || [ $NB_WARNINGS -ne 0 ]
        then
            details_table "$1" $2 $4
        fi
    else
        echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
        echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
        echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    fi
    echo "      </tr>" >> ./build_results.html
}

function summary_table_footer {
    echo "   </table>" >> ./build_results.html
}

function sca_summary_table_header {
    echo "   <h3>$1</h3>" >> ./build_results.html
    echo "   <table border = \"1\">" >> ./build_results.html
    echo "      <tr bgcolor = \"#33CCFF\" >" >> ./build_results.html
    echo "        <th>Error / Warning Type</th>" >> ./build_results.html
    echo "        <th>Nb Errors</th>" >> ./build_results.html
    echo "        <th>Nb Warnings</th>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
    echo "0" > ccp_error_cnt.txt
}

function sca_summary_table_row {
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >$2</td>" >> ./build_results.html
    if [ -f $1 ]
    then
        NB_ERRORS=`egrep "severity=\"error\"" $1 | egrep -c "id=\"$3\""`
        echo "        <td>$NB_ERRORS</td>" >> ./build_results.html
        echo "        <td>N/A</td>" >> ./build_results.html
        if [ -f ccp_error_cnt.txt ]
        then
            TOTAL_ERRORS=`cat ccp_error_cnt.txt`
            TOTAL_ERRORS=$((TOTAL_ERRORS + NB_ERRORS))
            echo $TOTAL_ERRORS > ccp_error_cnt.txt
        fi
    else
        echo "        <td>Unknown</td>" >> ./build_results.html
        echo "        <td>Unknown</td>" >> ./build_results.html
    fi
    echo "      </tr>" >> ./build_results.html
}

function sca_summary_table_footer {
    if [ -f $1 ]
    then
        NB_ERRORS=`egrep -c "severity=\"error\"" $1`
        NB_WARNINGS=`egrep -c "severity=\"warning\"" $1`
        if [ -f ccp_error_cnt.txt ]
        then
            echo "      <tr>" >> ./build_results.html
            echo "        <td bgcolor = \"lightcyan\" >Others</td>" >> ./build_results.html
            TOTAL_ERRORS=`cat ccp_error_cnt.txt`
            TOTAL_ERRORS=$((NB_ERRORS - TOTAL_ERRORS))
            echo "        <td>$TOTAL_ERRORS</td>" >> ./build_results.html
            echo "        <td>$NB_WARNINGS</td>" >> ./build_results.html
            echo "      </tr>" >> ./build_results.html
            rm -f ccp_error_cnt.txt
        fi
        echo "      <tr bgcolor = \"#33CCFF\" >" >> ./build_results.html
        echo "        <th>Total</th>" >> ./build_results.html
        echo "        <th>$NB_ERRORS</th>" >> ./build_results.html
        echo "        <th>$NB_WARNINGS</th>" >> ./build_results.html
    else
        echo "      <tr bgcolor = \"#33CCFF\"  >" >> ./build_results.html
        echo "        <th>Total</th>" >> ./build_results.html
        echo "        <th>Unknown</th>" >> ./build_results.html
        echo "        <th>Unknown</th>" >> ./build_results.html
        if [ -f ccp_error_cnt.txt ]
        then
            rm -f ccp_error_cnt.txt
        fi
    fi
    echo "      </tr>" >> ./build_results.html
    echo "   </table>" >> ./build_results.html
    echo "   <p>Full details in zipped artifact (cppcheck/cppcheck.xml) </p>" >> ./build_results.html
    echo "   <p>Graphical Interface tool : <code>cppcheck-gui -l cppcheck/cppcheck.xml</code> </p>" >> ./build_results.html
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

echo "<!DOCTYPE html>" > ./build_results.html
echo "<html class=\"no-js\" lang=\"en-US\">" >> ./build_results.html
echo "<head>" >> ./build_results.html
echo "  <title>Build Results for $JOB_NAME job build #$BUILD_ID</title>" >> ./build_results.html
echo "  <base href = \"http://www.openairinterface.org/\" />" >> ./build_results.html
echo "</head>" >> ./build_results.html
echo "<body>" >> ./build_results.html
echo "  <table style=\"border-collapse: collapse; border: none;\">" >> ./build_results.html
echo "    <tr style=\"border-collapse: collapse; border: none;\">" >> ./build_results.html
echo "      <td style=\"border-collapse: collapse; border: none;\">" >> ./build_results.html
echo "        <a href=\"http://www.openairinterface.org/\">" >> ./build_results.html
echo "           <img src=\"/wp-content/uploads/2016/03/cropped-oai_final_logo2.png\" alt=\"\" border=\"none\" height=50 width=150>" >> ./build_results.html
echo "           </img>" >> ./build_results.html
echo "        </a>" >> ./build_results.html
echo "      </td>" >> ./build_results.html
echo "      <td style=\"border-collapse: collapse; border: none; vertical-align: center;\">" >> ./build_results.html
echo "        <b><font size = \"6\">Job Summary -- Job: $JOB_NAME -- Build-ID: $BUILD_ID</font></b>" >> ./build_results.html
echo "      </td>" >> ./build_results.html
echo "    </tr>" >> ./build_results.html
echo "  </table>" >> ./build_results.html
echo "  <br>" >> ./build_results.html
echo "   <table border = \"1\">" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >GIT Repository</td>" >> ./build_results.html
echo "        <td>$GIT_URL</td>" >> ./build_results.html
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Job Trigger</td>" >> ./build_results.html
if [ $PU_TRIG -eq 1 ]; then echo "        <td>Push Event</td>" >> ./build_results.html; fi
if [ $MR_TRIG -eq 1 ]; then echo "        <td>Merge-Request</td>" >> ./build_results.html; fi
echo "      </tr>" >> ./build_results.html
if [ $PU_TRIG -eq 1 ]
then
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Branch</td>" >> ./build_results.html
    echo "        <td>$SOURCE_BRANCH</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Commit ID</td>" >> ./build_results.html
    echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
fi
if [ $MR_TRIG -eq 1 ]
then
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Source Branch</td>" >> ./build_results.html
    echo "        <td>$SOURCE_BRANCH</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Source Commit ID</td>" >> ./build_results.html
    echo "        <td>$SOURCE_COMMIT_ID</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Target Branch</td>" >> ./build_results.html
    echo "        <td>$TARGET_BRANCH</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Target Commit ID</td>" >> ./build_results.html
    echo "        <td>$TARGET_COMMIT_ID</td>" >> ./build_results.html
    echo "      </tr>" >> ./build_results.html
fi
echo "   </table>" >> ./build_results.html
echo "   <h2>Build Summary</h2>" >> ./build_results.html

if [ -f ./oai_rules_result.txt ]
then
    echo "   <h3>OAI Coding / Formatting Guidelines Check</h3>" >> ./build_results.html
    echo "   <table border = "1">" >> ./build_results.html
    echo "      <tr>" >> ./build_results.html
    echo "        <td bgcolor = \"lightcyan\" >Result:</td>" >> ./build_results.html
    NB_FILES=`cat ./oai_rules_result.txt`
    if [ $NB_FILES = "0" ]
    then 
        if [ $PU_TRIG -eq 1 ]; then echo "        <td bgcolor = \"green\">All files in repository follow OAI rules. </td>" >> ./build_results.html; fi
        if [ $MR_TRIG -eq 1 ]; then echo "        <td bgcolor = \"green\">All modified files in Merge-Request follow OAI rules.</td>" >> ./build_results.html; fi
    else
        if [ $PU_TRIG -eq 1 ]; then echo "        <td bgcolor = \"orange\">$NB_FILES files in repository DO NOT follow OAI rules. </td>" >> ./build_results.html; fi
        if [ $MR_TRIG -eq 1 ]; then echo "        <td bgcolor = \"orange\">$NB_FILES modified files in Merge-Request DO NOT follow OAI rules.</td>" >> ./build_results.html; fi
    fi
    echo "      </tr>" >> ./build_results.html
    echo "   </table>" >> ./build_results.html
fi

echo "   <h2>Ubuntu 16.04 LTS -- Summary</h2>" >> ./build_results.html

sca_summary_table_header "OAI Static Code Analysis with CPPCHECK"
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Uninitialized variable" uninitvar
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Uninitialized struct member" uninitStructMember
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Memory leak" memleak
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Memory is freed twice" doubleFree
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Resource leak" resourceLeak
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Possible null pointer dereference" nullPointer
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Array access  out of bounds" arrayIndexOutOfBounds
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Buffer is accessed out of bounds" bufferAccessOutOfBounds
sca_summary_table_row ./archives/cppcheck/cppcheck.xml "Expression depends on order of evaluation of side effects" unknownEvaluationOrder
sca_summary_table_footer ./archives/cppcheck/cppcheck.xml

summary_table_header "OAI Build eNB -- USRP option"
summary_table_row "LTE SoftModem - Release 15" ./archives/enb_usrp/lte-softmodem.Rel15.txt "Built target lte-softmodem" ./enb_usrp_row1.html
summary_table_row "Coding - Release 15" ./archives/enb_usrp/coding.Rel15.txt "Built target coding" ./enb_usrp_row2.html
summary_table_row "OAI USRP device if - Release 15" ./archives/enb_usrp/oai_usrpdevif.Rel15.txt "Built target oai_usrpdevif" ./enb_usrp_row3.html
summary_table_row "Parameters Lib Config - Release 15" ./archives/enb_usrp/params_libconfig.Rel15.txt "Built target params_libconfig" ./enb_usrp_row4.html
summary_table_footer

summary_table_header "OAI Build basic simulator option"
summary_table_row "Basic Simulator eNb - Release 15" ./archives/basic_sim/basic_simulator_enb.txt "Built target lte-softmodem" ./basic_sim_row1.html
summary_table_row "Basic Simulator UE - Release 15" ./archives/basic_sim/basic_simulator_ue.txt "Built target lte-uesoftmodem" ./basic_sim_row2.html
summary_table_row "Conf 2 UE data - Release 15" ./archives/basic_sim/conf2uedata.Rel15.txt "Built target conf2uedata" ./basic_sim_row3.html
summary_table_footer

summary_table_header "OAI Build Physical simulators option"
summary_table_row "DL Simulator - Release 15" ./archives/phy_sim/dlsim.Rel15.txt "Built target dlsim" ./phy_sim_row1.html
summary_table_row "UL Simulator - Release 15" ./archives/phy_sim/ulsim.Rel15.txt "Built target ulsim" ./phy_sim_row2.html
summary_table_row "Coding - Release 15" ./archives/phy_sim/coding.Rel15.txt "Built target coding" ./phy_sim_row3.html
if [ -f ./archives/phy_sim/./archives/phy_sim/ldpctest.Rel15.txt ]
then
    summary_table_row "LDPC Test - Release 15" ./archives/phy_sim/ldpctest.Rel15.txt "Built target coding" ./phy_sim_row4.html
fi
if [ -f ./archives/phy_sim/./archives/phy_sim/polartest.Rel15.txt ]
then
    summary_table_row "Polar Test - Release 15" ./archives/phy_sim/polartest.Rel15.txt "Built target coding" ./phy_sim_row5.html
fi
summary_table_footer


if [ -f archives/gnb_usrp/nr-softmodem.Rel15.txt ]
then
    summary_table_header "OAI Build gNB -- USRP option"
    summary_table_row "LTE SoftModem - Release 15" ./archives/gnb_usrp/nr-softmodem.Rel15.txt "Built target nr-softmodem" ./gnb_usrp_row1.html
    summary_table_row "Coding - Release 15" ./archives/gnb_usrp/coding.Rel15.txt "Built target coding" ./gnb_usrp_row2.html
    summary_table_row "OAI USRP device if - Release 15" ./archives/gnb_usrp/oai_usrpdevif.Rel15.txt "Built target oai_usrpdevif" ./gnb_usrp_row3.html
    summary_table_row "Parameters Lib Config - Release 15" ./archives/gnb_usrp/params_libconfig.Rel15.txt "Built target params_libconfig" ./gnb_usrp_row4.html
    summary_table_footer
fi

if [ -f archives/nrue_usrp/nr-uesoftmodem.Rel15.txt ]
then
    summary_table_header "OAI Build 5G NR UE -- USRP option"
    summary_table_row "UE SoftModem - Release 15" ./archives/nrue_usrp/nr-uesoftmodem.Rel15.txt "Built target nr-uesoftmodem" ./nrue_usrp_row1.html
    summary_table_row "Coding - Release 15" ./archives/nrue_usrp/coding.Rel15.txt "Built target coding" ./nrue_usrp_row2.html
    summary_table_row "OAI USRP device if - Release 15" ./archives/nrue_usrp/oai_usrpdevif.Rel15.txt "Built target oai_usrpdevif" ./nrue_usrp_row3.html
    summary_table_row "Parameters Lib Config - Release 15" ./archives/nrue_usrp/params_libconfig.Rel15.txt "Built target params_libconfig" ./nrue_usrp_row4.html
    summary_table_footer
fi

summary_table_header "OAI Build eNB -- ETHERNET transport option"
summary_table_row "LTE SoftModem w/o S1 - Release 15" ./archives/enb_eth/lte-softmodem-nos1.Rel15.txt "Built target lte-softmodem" ./enb_eth_row1.html
summary_table_row "Coding - Release 15" ./archives/enb_eth/coding.Rel15.txt "Built target coding" ./enb_eth_row2.html
summary_table_row "OAI ETHERNET transport - Release 15" ./archives/enb_eth/oai_eth_transpro.Rel15.txt "Built target oai_eth_transpro" ./enb_eth_row3.html
summary_table_row "Parameters Lib Config - Release 15" ./archives/enb_eth/params_libconfig.Rel15.txt "Built target params_libconfig" ./enb_eth_row4.html
summary_table_row "RB Tools - Release 15" ./archives/enb_eth/rb_tool.Rel15.txt "Built target rb_tool" ./enb_eth_row5.html
summary_table_row "NAS Mesh - Release 15" ./archives/enb_eth/nasmesh.Rel15.txt "Built target nasmesh" ./enb_eth_row6.html
summary_table_footer

summary_table_header "OAI Build UE -- ETHERNET transport option"
summary_table_row "LTE UE SoftModem w/o S1 - Release 15" ./archives/ue_eth/lte-uesoftmodem-nos1.Rel15.txt "Built target lte-uesoftmodem" ./ue_eth_row1.html
summary_table_row "Coding - Release 15" ./archives/ue_eth/coding.Rel15.txt "Built target coding" ./ue_eth_row2.html
summary_table_row "OAI ETHERNET transport - Release 15" ./archives/ue_eth/oai_eth_transpro.Rel15.txt "Built target oai_eth_transpro" ./ue_eth_row3.html
summary_table_row "Parameters Lib Config - Release 15" ./archives/ue_eth/params_libconfig.Rel15.txt "Built target params_libconfig" ./ue_eth_row4.html
summary_table_row "RB Tools - Release 15" ./archives/ue_eth/rb_tool.Rel15.txt "Built target rb_tool" ./ue_eth_row5.html
summary_table_row "NAS Mesh - Release 15" ./archives/ue_eth/nasmesh.Rel15.txt "Built target nasmesh" ./ue_eth_row6.html
summary_table_footer

echo "   <h2>Red Hat (CentOS Linux release 7.4.1708) -- Summary</h2>" >> ./build_results.html

summary_table_header "Red Hat -- OAI Build eNB -- USRP option"
summary_table_row "LTE SoftModem - Release 15" ./archives/red_hat/lte-softmodem.Rel15.txt "Built target lte-softmodem" ./enb_usrp_rh_row1.html
summary_table_row "Coding - Release 15" ./archives/red_hat/coding.Rel15.txt "Built target coding" ./enb_usrp_rh_row2.html
summary_table_row "OAI USRP device if - Release 15" ./archives/red_hat/oai_usrpdevif.Rel15.txt "Built target oai_usrpdevif" ./enb_usrp_rh_row3.html
summary_table_row "Parameters Lib Config - Release 15" ./archives/red_hat/params_libconfig.Rel15.txt "Built target params_libconfig" ./enb_usrp_rh_row4.html
summary_table_footer

echo "   <h3>Details</h3>" >> ./build_results.html

for DETAILS_TABLE in `ls ./enb_usrp_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./basic_sim_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./phy_sim_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done

for DETAILS_TABLE in `ls ./gnb_usrp_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./nrue_usrp_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./enb_eth_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./ue_eth_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
for DETAILS_TABLE in `ls ./enb_usrp_rh_row*.html`
do
    cat $DETAILS_TABLE >> ./build_results.html
done
rm -f ./enb_usrp_row*.html ./basic_sim_row*.html ./phy_sim_row*.html ./enb_eth_row*.html ./ue_eth_row*.html ./enb_usrp_rh_row*.html

echo "</body>" >> ./build_results.html
echo "</html>" >> ./build_results.html

exit 0
