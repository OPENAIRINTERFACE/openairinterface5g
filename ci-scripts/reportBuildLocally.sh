#!/bin/bash

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
echo "</head>" >> ./build_results.html
echo "<body>" >> ./build_results.html
echo "   <h1>Job Summary -- Job: $JOB_NAME -- Build-ID: $BUILD_ID</h1>" >> ./build_results.html
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

echo "   <h3>OAI Build eNb -- USRP option</h3>" >> ./build_results.html
echo "   <table border = "1">" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <th>Element</th>" >> ./build_results.html
echo "        <th>Status</th>" >> ./build_results.html
echo "        <th>Nb Errors</th>" >> ./build_results.html
echo "        <th>Nb Warnings</th>" >> ./build_results.html
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >LTE SoftModem - Release 14</th>" >> ./build_results.html
if [ -f ./archives/enb_usrp/lte-softmodem.Rel14.txt ]
then
    STATUS=`egrep -c "Built target lte-softmodem" ./archives/enb_usrp/lte-softmodem.Rel14.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/enb_usrp/lte-softmodem.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/enb_usrp/lte-softmodem.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Coding - Release 14</th>" >> ./build_results.html
if [ -f ./archives/enb_usrp/coding.Rel14.txt ]
then
    STATUS=`egrep -c "Built target coding" ./archives/enb_usrp/coding.Rel14.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/enb_usrp/coding.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/enb_usrp/coding.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >OAI USRP device if - Release 14</th>" >> ./build_results.html
if [ -f ./archives/enb_usrp/oai_usrpdevif.Rel14.txt ]
then
    STATUS=`egrep -c "Built target oai_usrpdevif" ./archives/enb_usrp/oai_usrpdevif.Rel14.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/enb_usrp/oai_usrpdevif.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/enb_usrp/oai_usrpdevif.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Parameters Lib Config - Release 14</th>" >> ./build_results.html
if [ -f ./archives/enb_usrp/params_libconfig.Rel14.txt ]
then
    STATUS=`egrep -c "Built target params_libconfig" ./archives/enb_usrp/params_libconfig.Rel14.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/enb_usrp/params_libconfig.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/enb_usrp/params_libconfig.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "   </table>" >> ./build_results.html

# conf2uedata.Rel14.txt
# archives/basic_sim

echo "   <h3>OAI Build basic simulator option</h3>" >> ./build_results.html
echo "   <table border = "1">" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <th>Element</th>" >> ./build_results.html
echo "        <th>Status</th>" >> ./build_results.html
echo "        <th>Nb Errors</th>" >> ./build_results.html
echo "        <th>Nb Warnings</th>" >> ./build_results.html
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Basic Simulator eNb - Release 14</th>" >> ./build_results.html
if [ -f ./archives/basic_sim/basic_simulator_enb.txt ]
then
    STATUS=`egrep -c "Built target lte-softmodem" ./archives/basic_sim/basic_simulator_enb.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/basic_sim/basic_simulator_enb.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/basic_sim/basic_simulator_enb.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Basic Simulator UE - Release 14</th>" >> ./build_results.html
if [ -f ./archives/basic_sim/basic_simulator_ue.txt ]
then
    STATUS=`egrep -c "Built target lte-uesoftmodem" ./archives/basic_sim/basic_simulator_ue.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/basic_sim/basic_simulator_ue.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/basic_sim/basic_simulator_ue.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "      <tr>" >> ./build_results.html
echo "        <td bgcolor = \"lightcyan\" >Conf 2 UE data - Release 14</th>" >> ./build_results.html
if [ -f ./archives/basic_sim/conf2uedata.Rel14.txt ]
then
    STATUS=`egrep -c "Built target conf2uedata" ./archives/basic_sim/conf2uedata.Rel14.txt`
    if [ $STATUS -eq 1 ]
    then
        echo "        <td bgcolor = \"green\" >OK</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >KO</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "error:" ./archives/basic_sim/conf2uedata.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"red\" >$STATUS</th>" >> ./build_results.html
    fi
    STATUS=`egrep -c "warning:" ./archives/basic_sim/conf2uedata.Rel14.txt`
    if [ $STATUS -eq 0 ]
    then
        echo "        <td bgcolor = \"green\" >$STATUS</th>" >> ./build_results.html
    else
        echo "        <td bgcolor = \"orange\" >$STATUS</th>" >> ./build_results.html
    fi
else
    echo "        <td bgcolor = \"lightgray\" >Unknown</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
    echo "        <td bgcolor = \"lightgray\" >--</th>" >> ./build_results.html
fi
echo "      </tr>" >> ./build_results.html
echo "   </table>" >> ./build_results.html

echo "</body>" >> ./build_results.html
echo "</html>" >> ./build_results.html

exit 0
