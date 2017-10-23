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
################################################################################
# file build_oai.bash
# brief OAI automated build tool that can be used to install, compile, run OAI.
# author  Navid Nikaein, Lionel GAUTHIER
# company Eurecom
# email:  navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr 
#

#!/bin/bash
################################
# include helper functions
################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
. $THIS_SCRIPT_PATH/build_helper.bash

check_for_root_rights

#######################################
# Default PARAMETERS
######################################
declare OAI_DB_ADMIN_USER_NAME="root"
declare OAI_DB_ADMIN_USER_PASSWORD="linux"

#only one could be set at the time
declare BUILD_LTE="NONE" # ENB, EPC, HSS

declare HW="EXMIMO" # EXMIMO, USRP, ETHERNET, NONE
declare TARGET="ALL" # ALL, SOFTMODEM, OAISIM, UNISIM, NONE
declare ENB_S1=1
declare REL="REL8" # REL8, REL10
declare RT="NONE" # RTAI, RT_PREMPT, NONE
declare DEBUG=0
declare CONFIG_FILE=" "
declare CONFIG_FILE_ACCESS_OK=0
declare EXE_ARGUMENTS=" "
declare RUN_GDB=0
declare RUN=0
declare DISABLE_CHECK_INSTALLED_SOFTWARE=0
declare OAI_CLEAN=0
declare CLEAN_IPTABLES=0
declare CLEAN_HSS=0

declare OAI_TEST=0
declare XFORMS=0

# script is not currently handling these params
declare EPC=0 # flag to build EPC

declare ITTI_ANALYZER=0
declare VCD_TIMING=0
declare WIRESHARK=0
declare TIME_MEAS=0
declare DOXYGEN=0
declare DEV=0

#EMULATION_DEV_INTERFACE="eth0"
#EMULATION_MULTICAST_GROUP=1
#EMULATION_DEV_ADDRESS=`ifconfig $EMULATION_DEV_INTERFACE | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`

############## script params #####################

if [ -f ./.lock_oaibuild ]; then 
    OAI_CLEAN=0
else 
    OAI_CLEAN=1
fi 
 
#for i in "$@"
#do 
#    echo "i is : $i"
#    case $i in


  until [ -z "$1" ]
  do
  case "$1" in
       -a | --doxygen)
            DOXYGEN=1
            echo "setting doxygen flag to: $DOXYGEN"
            shift;
            ;;
       -b | --disable-s1)
            ENB_S1=0
            echo "disable eNB S1 flag"
            shift;
            ;;
       -c | --clean)
            rm -rf ./.lock_oaibuild
            OAI_CLEAN=1
            CLEAN_HSS=1
            echo "setting clean flag to: $OAI_CLEAN"
            echo "may check package installation, and recompile OAI"
            shift;
            ;;
       --clean-iptables)
            CLEAN_IPTABLES=1;
            shift;
            ;;
       -C | --config-file)
            CONFIG_FILE=$2
            # may be relative path 
            if [ -f $(dirname $(readlink -f $0))/$CONFIG_FILE ]; then
                CONFIG_FILE=$(dirname $(readlink -f $0))/$CONFIG_FILE
                echo "setting config file to: $CONFIG_FILE"
                CONFIG_FILE_ACCESS_OK=1
            else
                # may be absolute path 
                if [ -f $CONFIG_FILE ]; then
                    echo "setting config file to: $CONFIG_FILE"
                    CONFIG_FILE_ACCESS_OK=1
                else
                    echo "config file not found"
                    exit 1
                fi
            fi
            EXE_ARGUMENTS="$EXE_ARGUMENTS -O $CONFIG_FILE"
            shift 2;
            ;;
       -d | --debug)
            DEBUG=1
            echo "setting debug flag to: $DEBUG"
            shift;
            ;;
       -D | --disable-check-installed-software)
            DISABLE_CHECK_INSTALLED_SOFTWARE=1
            echo "disable check installed software"
            shift;
            ;;
       -e | --realtime)
            RT=$2
            echo "setting realtime flag to: $RT"
            shift 2 ;
            ;;
       -g | --run-with-gdb)
            DEBUG=1
            RUN=1 
            RUN_GDB=1
            echo "Running with gdb"
            shift;
            ;;
       -K | --itti-dump-file)
            ITTI_ANALYZER=1
            ITTI_DUMP_FILE=$2
            echo "setting ITTI dump file to: $ITTI_DUMP_FILE"
            EXE_ARGUMENTS="$EXE_ARGUMENTS -K $ITTI_DUMP_FILE"
            shift 2;
            ;;
       -l | --build-target) 
            BUILD_LTE=$2
            echo "setting top-level build target to: $2"
            shift 2;
            ;;
       -h | --help)
            print_help
            exit -1
            ;;
       -m | --build-from-makefile)
            BUILD_FROM_MAKEFILE=1
            set_build_from_makefile $BUILD_FROM_MAKEFILE
            echo "setting a flag to build from makefile to: $BUILD_FROM_MAKEFILE"
            shift;
            ;;
       -r | --3gpp-release)
            REL=$2 
            echo "setting release to: $REL"
            shift 2 ;
            ;;
       -R | --run)
            RUN=1 
            echo "setting run to $RUN"
            shift;
            ;;
       -s | --check)
            OAI_TEST=1
            echo "setting sanity check to: $OAI_TEST"
            shift;
            ;;
       -t | --enb-build-target)
            TARGET=$2 
            echo "setting enb build target to: $TARGET"
            shift 2;
            ;;
       -V | --vcd)
            echo "setting gtk-wave output"
            VCD_TIMING=1
            EXE_ARGUMENTS="$EXE_ARGUMENTS -V"
            shift ;
            ;;
       -w | --hardware)
            HW="$2" #"${i#*=}"
            echo "setting hardware to: $HW"
            shift 2 ;
            ;;
       -x | --xforms)
            XFORMS=1
            EXE_ARGUMENTS="$EXE_ARGUMENTS -d"
            echo "setting xforms to: $XFORMS"
            shift;
            ;;
       -p | --wireshark)
	  WIRESHARK=1
	  echo "enabling Wireshark interface to $WIRESHARK"
          shift;
          ;;
       -z | --defaults)
            echo "setting all parameters to: default"
            rm -rf ./.lock_oaibuild
            OAI_CLEAN=1
            HW="EXMIMO"
            TARGET="ALL" 
            ENB_S1=1
            REL="REL8" 
            RT="NONE"
            DEBUG=0
            ENB_CONFIG_FILE=$OPENAIR_TARGETS/"PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.conf"
            OAI_TEST=0
            shift ;
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
echo_info "1. Creating the bin dir ..." 
mkdir -m 777 bin 

build_date=`date +%Y_%m_%d`
oai_build_date="oai_built_${build_date}"
touch bin/${oai_build_date} 
chmod -f 777 bin/${oai_build_date}

touch bin/install_log.txt
chmod -f 777 bin/install_log.txt
################################
# cleanup first 
################################
#echo_info "3. Cleaning ..."

#$SUDO kill -9 `ps -ef | grep oaisim | awk '{print $2}'` 2>&1
#$SUDO kill -9 `ps -ef | grep lte-softmodem | awk '{print $2}'`  2>&1
#$SUDO kill -9 `ps -ef | grep dlsim | awk '{print $2}'`  2>&1
#$SUDO kill -9 `ps -ef | grep ulsim | awk '{print $2}'`  2>&1

if [ $CLEAN_IPTABLES -eq 1 ]; then 
    echo_info "Flushing iptables..."
    $SUDO modprobe ip_tables
    $SUDO modprobe x_tables
    $SUDO iptables -P INPUT ACCEPT
    $SUDO iptables -F INPUT
    $SUDO iptables -P OUTPUT ACCEPT
    $SUDO iptables -F OUTPUT
    $SUDO iptables -P FORWARD ACCEPT
    $SUDO iptables -F FORWARD
    $SUDO iptables -t nat -F
    $SUDO iptables -t mangle -F
    $SUDO iptables -t filter -F
    $SUDO iptables -t raw -F
    echo_info "Flushed iptables"
fi
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


echo "OPENAIR_HOME    = $OPENAIR_HOME" >>  bin/${oai_build_date}
echo "OPENAIR1_DIR    = $OPENAIR1_DIR"  >>  bin/${oai_build_date}
echo "OPENAIR2_DIR    = $OPENAIR2_DIR"  >>  bin/${oai_build_date}
echo "OPENAIR3_DIR    = $OPENAIR3_DIR"  >>  bin/${oai_build_date}
echo "OPENAIR3_DIR   = $OPENAIR3_DIR"  >>  bin/${oai_build_date}
echo "OPENAIR_TARGETS = $OPENAIR_TARGETS"  >>  bin/${oai_build_date}


build_enb(){

##########################################
# process parameters
#########################################

    echo_info "4. Process the parameters"

    echo_info "User-defined Parameters :  HW=$HW, TARGET=$TARGET, ENB_S1=$ENB_S1, REL=$REL, RT=$RT, DEBUG=$DEBUG XFORMS=$XFORMS"
    
    echo "User-defined Parameters :  HW=$HW, TARGET=$TARGET, ENB_S1=$ENB_S1, REL=$REL, RT=$RT, DEBUG=$DEBUG XFORMS=$XFORMS" >> bin/${oai_build_date}
    
 
############################################
# compilation directives 
############################################

    echo_info "5. building the compilation directives ..."
    
    
    SOFTMODEM_DIRECTIVES="DEBUG=$DEBUG XFORMS=$XFORMS "
    OAISIM_DIRECTIVES="DEBUG=$DEBUG XFORMS=$XFORMS "
    
    if [ $ENB_S1 -eq 1 ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES USE_MME=R10 ENABLE_ITTI=1 LINK_ENB_PDCP_TO_GTPV1U=1  SECU=1 "
        #OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES USE_MME=R10 ENABLE_ITTI=1 LINK_ENB_PDCP_TO_GTPV1U=1  SECU=1 "
	OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES ENABLE_ITTI=1 LINK_ENB_PDCP_TO_GTPV1U=1  SECU=1 "
    fi 
    
    if [ $DEBUG -eq 0 ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES DISABLE_XER_PRINT=1 "
        OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES DISABLE_XER_PRINT=1 "
    fi 

    if [ $HW = "USRP" ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES USRP=1 "
    fi
    
    if [ $HW = "EXMIMO" ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES EXMIMO=1 "
    fi
    
    if [ $HW = "ETHERNET" ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES ETHERNET=1 "
    fi 
    
    if [ $ENB_S1 -eq 0 ]; then 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES NAS=1 "
        OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES NAS=1 "
    fi 
    
    if [ $REL = "REL8" ]; then
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES Rel8=1 "
        OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES Rel8=1 "
    else 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES Rel10=1 "
        OAISIM_DIRECTIVES="$OAISIM_DIRECTIVES Rel10=1 "
    fi
    
    if [ $RT = "RTAI" ]; then 
        if [ ! -f /usr/realtime/modules/rtai_hal.ko ];   then
            echo_warning "RTAI doesn't seem to be installed"
            RT="NONE"
            SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES RTAI=0 "
        else 
            SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES HARD_RT=1 "
        fi
    else 
        SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES RTAI=0 "
        RT="NONE"
    fi
    
    if [ $TARGET != "ALL" ]; then 
        if [ $TARGET  != "SOFTMODEM" ]; then 
            HW="NONE"
        fi
    fi
    
    if [ $UBUNTU_REL = "12.04" ]; then 
        output=$(check_for_machine_type 2>&1) 
        MACHINE_ARCH=$?
        if [ $MACHINE_ARCH -eq 64 ]; then
            SOFTMODEM_DIRECTIVES="$SOFTMODEM_DIRECTIVES LIBCONFIG_LONG=1 "
            OAISIM_DIRECTIVES="$OASIM_DIRECTIVES LIBCONFIG_LONG=1 "
        fi
    fi
    
    echo_success "SOFTMODEM Compilation directives: $SOFTMODEM_DIRECTIVES"
    echo_success "OAISIM Compilation directives:    $OAISIM_DIRECTIVES"
    
    echo "SOFTMODEM Compilation directives: $SOFTMODEM_DIRECTIVES" >>  bin/${oai_build_date}
    echo "OAISIM Compilation directive:    $OAISIM_DIRECTIVES" >>  bin/${oai_build_date}
    
    
############################################
# check the installation
############################################
    if [ $DISABLE_CHECK_INSTALLED_SOFTWARE -eq 0 ]; then 
        echo_info "6. Checking the the required softwares/packages ..."

        check_install_oai_software  
        if [ $HW = "USRP" ]; then 
            check_install_usrp_uhd_driver 
        fi
        check_install_asn1c
        check_install_nettle
    else
        echo_info "6. Not checking the required softwares/packages ..."
        touch ./.lock_oaibuild
    fi
    
############################################
# compile 
############################################

    echo_info "7. compiling and installing the OAI binaries ..."

    softmodem_compiled=1
    oaisim_compiled=1
    unisim_compiled=1
    
    if [ $TARGET = "ALL" ]; then
        echo "############# compile_ltesoftmodem #############" >> bin/install_log.txt 
        output=$(compile_ltesoftmodem  >> bin/install_log.txt  2>&1 )
        softmodem_compiled=$?
        check_for_ltesoftmodem_executable
        echo_info "7.1 finished ltesoftmodem target : check the installation log file bin/install_log.txt" 

        echo "################ compile_oaisim #################"  >> bin/install_log.txt 
        output=$(compile_oaisim      >> bin/install_log.txt   2>&1 )
        oaisim_compiled=$?
        check_for_oaisim_executable
        echo_info "7.2 finished oaisim target : check the installation log file bin/install_log.txt" 

        echo "################## compile_unisim ##################"  >> bin/install_log.txt 
        output=$(compile_unisim      >> bin/install_log.txt  2>&1 )
        unisim_compiled=$?
        check_for_dlsim_executable
        check_for_ulsim_executable
        check_for_pucchsim_executable
        check_for_prachsim_executable
        check_for_pdcchsim_executable
        check_for_pbchsim_executable
        check_for_mbmssim_executable
        echo_info "7.3 finished unisim target : check the installation log file bin/install_log.txt" 


    else
        if [ $TARGET == "SOFTMODEM" ]; then 
            echo "############# compile_ltesoftmodem #############" >> bin/install_log.txt 
            output=$(compile_ltesoftmodem   >> bin/install_log.txt 2>&1 )
            softmodem_compiled=$?
            check_for_ltesoftmodem_executable
            echo_info "7.1 finished ltesoftmodem target: check the installation log file bin/install_log.txt"
            
            if [ $HW == "EXMIMO" ]; then 
                output=$(compile_exmimo2_driver   >> bin/install_log.txt  2>&1)
            fi
        fi
        if [ $TARGET = "OAISIM" ]; then 
            echo "################ compile_oaisim #################"  >> bin/install_log.txt 
            output=$(compile_oaisim   >> bin/install_log.txt 2>&1 )
            oaisim_compiled=$?	
            check_for_oaisim_executable
            echo_info "7.2 finished oaisim target: check the installation log file bin/install_log.txt" 
            
            if [ $ENB_S1 -eq 1 ]; then
                compile_nas_tools  2>&1
                nas_tools_compiled=$?  
                check_for_nas_tools_executable
                echo_info "7.2.1 finished nas ue target: check the installation log file bin/install_log.txt"
            fi
        fi
        if [ $TARGET = "UNISIM" ]; then 
           echo "################## compile_unisim ##################"  >> bin/install_log.txt 
            output=$(compile_unisim   >> bin/install_log.txt 2>&1 )
            unisim_compiled=$?
            check_for_dlsim_executable
            check_for_ulsim_executable
            check_for_pucchsim_executable
            check_for_prachsim_executable
            check_for_pdcchsim_executable
            check_for_pbchsim_executable
            check_for_mbmssim_executable
            echo_info "7.3 finished unisim target: check the installation log file bin/install_log.txt" 
        fi
    fi


############################################
# install 
############################################

    echo_info "8. Installing ..."
    
    if [ $softmodem_compiled -eq 0 ]; then 
        echo_success "target lte-softmodem built and installed in the bin directory"
        echo "target lte-softmodem built and installed in the bin directory"  >>  bin/${oai_build_date}
        output=$(install_ltesoftmodem $RT $HW $ENB_S1 )
    fi
    if [ $oaisim_compiled -eq 0 ]; then 
        echo_success "target oaisim built and installed in the bin directory"
        echo "target oaisim built and installed in the bin directory"  >>  bin/${oai_build_date}
        output=$(install_oaisim $ENB_S1 )
    fi 
    if [ $unisim_compiled -eq  0 ]; then 
        echo_success "target unisim built and installed in the bin directory"
        echo "target unisim built and installed in the bin directory"  >>  bin/${oai_build_date}
    fi 
    
    echo_info "build terminated, binaries are located in bin/"
    echo_info "build terminated, logs are located in bin/${oai_build_date} and bin/install_log.txt"
    


}
build_epc(){

    epc_compiled=1

    echo_info "Note: this scripts tested only on Ubuntu 14.04x64"

######################################
# CHECK MISC SOFTWARES AND LIBS      #
######################################
    if [ $DISABLE_CHECK_INSTALLED_SOFTWARE -eq 0 ]; then 
        echo_info "4. Checking the the required softwares/packages for EPC..."

        check_install_epc_software  
        check_install_asn1c
        if [ $OAI_CLEAN -eq 1 ]; then
            check_install_freediamter
        else 
            if [ ! -d /usr/local/etc/freeDiameter ]; then
                check_install_freediamter
            fi
        fi
    else
        echo_info "4. Not checking the required softwares/packages for EPC"
    fi

###########################################
# configure and compile
##########################################

    echo_info "5. configure and compile epc"

    output=$(compile_epc $OAI_CLEAN  >> $OPENAIR_TARGETS/bin/install_log.txt  2>&1 )
    epc_compiled=$?
    if [ $epc_compiled -ne 0 ]; then
        echo_error "EPC compilation failed : check the installation log file bin/install_log.txt" 
        exit 1
    fi
    check_for_epc_executable
    echo_info "finished epc target: check the installation log file bin/install_log.txt" 

    if [ $CONFIG_FILE_ACCESS_OK -eq 0 ]; then
        echo_error "You have to provide a EPC config file"
        exit 1
    fi
    
    TEMP_FILE=`tempfile`
    VARIABLES=" S6A_CONF\|\
           HSS_HOSTNAME\|\
           REALM"

    VARIABLES=$(echo $VARIABLES | sed -e 's/\\r//g')
    VARIABLES=$(echo $VARIABLES | tr -d ' ')
    cat $CONFIG_FILE | grep -w "$VARIABLES"| tr -d " " | tr -d ";" > $TEMP_FILE
    source $TEMP_FILE
    rm -f $TEMP_FILE

    if [ x"$REALM" == "x" ]; then
        echo_error "Your config file do not contain a REALM for S6A configuration"
        exit 1
    fi
    if [ x"$S6A_CONF" != "x./epc_s6a.conf" ]; then
        echo_error "Your config file do not contain the good path for the S6A config file,"
        echo_error "accordingly to what is done in this script, it should be set to epc_s6a.conf"
        exit 1
    fi

    check_epc_s6a_certificate $REALM

###########################################
# install the binary in bin
##########################################

    echo_info "6. install the binary file"

    if [ $epc_compiled -eq 0 ]; then 
        echo_success "target epc built and installed in the bin directory"
        echo "target epc built and installed in the bin directory"  >>  bin/${oai_build_date}
        cp -f $CONFIG_FILE  $OPENAIR_TARGETS/bin
        cp -f $OPENAIR3_DIR/objs/UTILS/CONF/s6a.conf  $OPENAIR_TARGETS/bin/epc_s6a.conf
    fi
}

build_hss(){
    echo_info "Note: this script tested only for Ubuntu 12.04 x64 -> 14.04 x64"

######################################
# CHECK MISC SOFTWARES AND LIBS      #
######################################
    if [ $DISABLE_CHECK_INSTALLED_SOFTWARE -eq 0 ]; then 
        echo_info "4. check the required packages for HSS"
        check_install_hss_software
        if [ $OAI_CLEAN -eq 1 ]; then
            check_install_freediamter
        else 
            if [ ! -d /usr/local/etc/freeDiameter ]; then
                check_install_freediamter
            fi
        fi
    else
        echo_info "4. Not checking the required packages for HSS"
    fi
    
  
######################################
# compile HSS                        #
######################################
    echo_info "5. compile HSS"

     # Bad behaviour of $OAI_CLEAN with ./.lock_oaibuild ...
     compile_hss $CLEAN_HSS
     hss_compiled=$?
     check_for_hss_executable
     echo_info "finished hss target" 
 
######################################
# Check certificates                 #
######################################
  
    TEMP_FILE=`tempfile`
    cat $OPENAIR3_DIR/OPENAIRHSS/conf/hss_fd.conf | grep -w "Identity" | tr -d " " | tr -d ";" > $TEMP_FILE
    cat $OPENAIR3_DIR/OPENAIRHSS/conf/hss.conf    | grep -w "MYSQL_user" | tr -d " " | tr -d ";" >> $TEMP_FILE
    cat $OPENAIR3_DIR/OPENAIRHSS/conf/hss.conf    | grep -w "MYSQL_pass" | tr -d " " | tr -d ";" >> $TEMP_FILE
    cat $OPENAIR3_DIR/OPENAIRHSS/conf/hss.conf    | grep -w "MYSQL_db" | tr -d " " | tr -d ";" >> $TEMP_FILE
    source $TEMP_FILE
    rm -f  $TEMP_FILE

    if [ x"$Identity" == "x" ]; then
        echo_error "Your config file do not contain a host identity for S6A configuration"
        exit 1
    fi
    HSS_REALM=$(echo $Identity | sed 's/.*\.//')
    HSS_HOSTNAME=${Identity%.$HSS_REALM}
    NEW_HOSTNAME=`hostname -s`
    if [ "x$HSS_HOSTNAME" != "x$NEW_HOSTNAME" ]; then
       echo_warning "Changing identity of HSS from <$HSS_HOSTNAME.$HSS_REALM> to <$NEW_HOSTNAME.$HSS_REALM>"
       sed -ibak "s/$HSS_HOSTNAME/$NEW_HOSTNAME/"  $OPENAIR3_DIR/OPENAIRHSS/conf/hss_fd.conf 
    fi
    check_hss_s6a_certificate $HSS_REALM
    
######################################
# fill the HSS DB
######################################
     echo_info "6. create HSS database (for EURECOM SIM CARDS)"
     hss_db_created=1
     create_hss_database $OAI_DB_ADMIN_USER_NAME $OAI_DB_ADMIN_USER_PASSWORD $MYSQL_user $MYSQL_pass $MYSQL_db
     if [ $? -eq 1 ]; then
         echo_fatal "hss DB not created"
     fi
}


 

echo_info "3. set the top-level build target"
case "$BUILD_LTE" in
    'ENB')
         echo_success "build LTE eNB"
         build_enb
         ;;
    'EPC')
         echo_success "build EPC(MME and xGW)"
         build_epc
         ;;
    'HSS')
        echo_success "build HSS"
        build_hss 
        ;;
    'NONE')
        ;;
    *)
        ;;
esac

# Additional operation 

############################################
# Generate doxygen documentation
############################################

if [ $DOXYGEN = 1 ]; then 
    echo_info "9. Generate doxygen documentation ..."
    doxygen $OPENAIR_TARGETS/DOCS/Doxyfile
    echo_info "9.1 use your navigator to open $OPENAIR_TARGETS/DOCS/html/index.html "
else 
    echo_info "9. Bypassing doxygen documentation ..."
fi 


############################################
# testing
############################################
    
if [ $OAI_TEST -eq 1 ]; then 
    echo_info "10. Running OAI pre commit tests (pre-ci) ..."
    python $OPENAIR_TARGETS/TEST/OAI/test01.py -l 
else 
    echo_info "10. Bypassing the Tests ..."
fi 
    
############################################
# run 
############################################

if [ $RUN -ne 0 ]; then 
    echo_info "11. Running ..."
    cd $OPENAIR_TARGETS/bin
    case "$BUILD_LTE" in
        'ENB')
            if [ $TARGET == "SOFTMODEM" ]; then 
                if [ $HW == "EXMIMO" ]; then 
                    $SUDO chmod 777 $OPENAIR_TARGETS/RT/USER/init_exmimo2.sh
                    $SUDO $OPENAIR_TARGETS/RT/USER/init_exmimo2.sh
                fi
		if [ $WIRESHARK -eq 1 ]; then 
		    EXE_ARGUMENTS="$EXE_ARGUMENTS -W"
		fi 
                echo "############# running ltesoftmodem #############"
                if [ $RUN_GDB -eq 0 ]; then 
                    $SUDO $OPENAIR_TARGETS/bin/lte-softmodem  `echo $EXE_ARGUMENTS`
                else
                    $SUDO setenv MALLOC_CHECK_ 2
                    $SUDO touch ~/.gdb_lte_softmodem
                    $SUDO echo "file $OPENAIR_TARGETS/bin/lte-softmodem" > ~/.gdb_lte_softmodem
                    $SUDO echo "set args $EXE_ARGUMENTS" >> ~/.gdb_lte_softmodem
                    $SUDO echo "run" >> ~/.gdb_lte_softmodem
                    $SUDO gdb -nh -x ~/.gdb_lte_softmodem 2>&1 
                fi
                
            elif [ $TARGET == "OAISIM" ]; then
            
                if [ $ENB_S1 -eq 0 ]; then
		    install_nasmesh
                else
                    # prepare NAS for UE
                    if [ ! -f .ue.nvram0 ]; then
                        echo_success "generate .ue_emm.nvram0 .ue.nvram0"
                        $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --gen
                    fi

                    if [ ! -f .usim.nvram0 ]; then
                        echo_success "generate .usim.nvram0"
                        $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --gen
                    fi
                    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --print
                    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --print

                    insmod  $OPENAIR2_DIR/NETWORK_DRIVER/UE_IP/ue_ip.ko
                    
                fi
                if [ $WIRESHARK -eq 1 ]; then 
		    EXE_ARGUMENTS="$EXE_ARGUMENTS -P wireshark"
		fi 
                if [ $RUN_GDB -eq 0 ]; then 
                    $SUDO exec $OPENAIR_TARGETS/bin/oaisim  `echo $EXE_ARGUMENTS`
                else
                    $SUDO setenv MALLOC_CHECK_ 2
                    $SUDO touch ~/.gdb_oaisim
                    $SUDO echo "file $OPENAIR_TARGETS/bin/lte-oaisim" > ~/.gdb_oaisim
                    $SUDO echo "set args $EXE_ARGUMENTS" >> ~/.gdb_oaisim
                    $SUDO echo "run" >> ~/.gdb_oaisim
                    $SUDO gdb -nh -x ~/.gdb_oaisim 2>&1 
                fi
            fi
            ;;
        
        
        'EPC')
            echo "############# running EPC #############"
            test_is_host_reachable $HSS_HOSTNAME.$REALM HSS
            if [ $RUN_GDB -eq 0 ]; then
                $SUDO $OPENAIR_TARGETS/bin/oai_epc  `echo $EXE_ARGUMENTS`
            else
                touch ~/.gdb_epc
                chmod 777 ~/.gdb_epc
                echo "file $OPENAIR_TARGETS/bin/oai_epc" > ~/.gdb_epc
                echo "set args $EXE_ARGUMENTS" >> ~/.gdb_epc
                echo "run" >> ~/.gdb_epc
                $SUDO gdb -nh -x ~/.gdb_epc 2>&1 
            fi
            ;;
        
        
        'HSS')
            echo "############# running HSS #############"
            cd $OPENAIR3_DIR/OPENAIRHSS/objs
            if [ $RUN_GDB -eq 0 ]; then
                $SUDO exec ./openair-hss -c ./conf/hss.conf
            else
                touch ~/.gdb_hss
                chmod 777 ~/.gdb_hss
                echo "file ./openair-hss" > ~/.gdb_hss
                echo "set args -c ./conf/hss.conf" >> ~/.gdb_hss
                echo "run" >> ~/.gdb_hss
                $SUDO gdb -nh -x ~/.gdb_hss 2>&1 
            fi
             ;;
         
         
        'NONE')
             ;;
         
         
        *)
             echo_error "Unknown option $BUILD_LTE: do not execute"
             ;;
    esac
else
    echo_info "11. No run requested, end of script"
    exit 0
fi


