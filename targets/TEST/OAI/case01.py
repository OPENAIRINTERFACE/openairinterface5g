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

# \file case01.py
# \brief test case 01 for OAI: compilations
# \author Navid Nikaein
# \date 2013 - 2015
# \version 0.1
# @ingroup _test

import log
import openair
import core

makerr1 = '***'
makerr2 = 'Error 1'

def execute(oai, user, pw, host, logfile,logdir,debug,timeout):
    
    case = '01'
    rv = 1
    oai.send_recv('cd $OPENAIR_TARGETS;')   
 
    try:
        log.start()
        test = '00'
        name = 'Check oai.svn.add'
        conf = 'svn st -q | grep makefile'
        diag = 'Makefile(s) changed. If you are adding a new file, make sure that it is added to the svn'
        rsp = oai.send_recv('svn st -q | grep -i makefile;') 
        for item in rsp.split("\n"):
            if "Makefile" in item:
                rsp2=item.strip() + '\n'
        oai.find_false_re(rsp,'Makefile')
    except log.err, e:
        diag = diag + "\n" + rsp2  
               #log.skip(case, test, name, conf, e.value, logfile)
        log.skip(case, test, name, conf, '', diag, logfile)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    oai.send('cd SIMU/USER;')   
    oai.send('mkdir ' + logdir + ';')
    
    try:
        log.start()
        test = '01'
        name = 'Compile oai.rel8.make'
        conf = 'make'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = "check the compilation errors for oai"
        oai.send('make cleanall;')
        oai.send('make cleanasn1;')
        oai.send('rm -f ./oaisim.rel8.'+host)
        oai.send_expect_false('make -j4 JF=1' + tee, makerr1, timeout)
        oai.send('cp ./oaisim ./oaisim.rel8.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '02'
        name = 'Compile oai.rel8.nas.make'
        conf = 'make nasmesh_fix; make LINK_ENB_PDCP_TO_IP_DRIVER=1'
        diag = 'check the compilation errors for oai and nas driver'
        oai.send('make cleanall;')
        oai.send('rm -f ./oaisim.rel8.nas'+host)
        oai.send('rm -f ./nasmesh;')
        oai.send('make nasmesh_clean;')
        trace = logdir + '/log_' + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('make nasmesh_fix' + tee, makerr1,  60)
        oai.send('cp $OPENAIR2_DIR/NAS/DRIVER/MESH/nasmesh.ko .')
        trace = logdir + '/log_' + case + test + '_2.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('make LINK_ENB_PDCP_TO_IP_DRIVER=1 JF=1 -j4' + tee, makerr1, timeout)
        oai.send('cp ./oaisim ./oaisim.rel8.nas.'+host)
        
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    oai.send('cd $OPENAIR_TARGETS;')     
    oai.send('cd RT/USER;')   

    try:
        log.start()
        test = '03'
        name = 'Compile oai.rel8.rf.make' 
        conf = 'make RTAI=0 EXMIMO=1 Rel8=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for Rel8'
        oai.send('make cleanall;')
        oai.send('rm -f ./oaisim.rel8.rf.'+host)
        oai.send_expect_false('make RTAI=0 EXMIMO=1 -j4' + tee, makerr1,  timeout)
        oai.send('cp ./oaisim ./oaisim.rel8.rf.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
        
    oai.send('cd $OPENAIR1_DIR;')     
    oai.send('cd SIMULATION/LTE_PHY;')   

    try:
        log.start()
        test = '04'
        name = 'Compile oai.rel8.phy.dlsim.make' 
        conf = 'make dlsim'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for dlsim in $OPENAIR1_DIR/SIMULATION/LTE_PHY'
        oai.send('make clean;')
        oai.send('rm -f ./dlsim.rel8.'+host)
        oai.send_expect_false('make dlsim -j4 PERFECT_CE=1' + tee, makerr1,  timeout)
        oai.send('cp ./dlsim ./dlsim.rel8.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '05'
        name = 'Compile oai.rel8.phy.ulsim.make' 
        conf = 'make ulsim'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ulsim in $OPENAIR1_DIR/SIMULATION/LTE_PHY'
        oai.send('make clean;')
        oai.send('rm -f ./ulsim.rel8.'+host)
        oai.send_expect_false('make ulsim -j4' + tee, makerr1,  timeout)
        oai.send('cp ./ulsim ./ulsim.rel8.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    oai.send('cd $OPENAIR_TARGETS;')     
    oai.send('cd SIMU/USER;')   
    
    try:
        log.start()
        test = '06'
        name = 'Compile oai.rel8.itti.make' 
        conf = 'make DISABLE_XER_PRINT=1 ENABLE_ITTI=1 Rel8=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ITTI Rel8'
        oai.send('make clean;')
        oai.send('rm -f ./oaisim.rel8.itti.'+host)
        oai.send_expect_false('make DISABLE_XER_PRINT=1 ENABLE_ITTI=1 Rel8=1 -j4' + tee, makerr1, timeout)
        oai.send('cp ./oaisim ./oaisim.rel8.itti.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '07'
        name = 'Compile oai.rel10.make' 
        conf = 'make RLC_STOP_ON_LOST_PDU=1 Rel10=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for Rel10'
        oai.send('make clean;')
        oai.send('make cleanall;')
        oai.send('make cleanasn1;')
        oai.send('rm -f ./oaisim.rel10.'+host)
        oai.send_expect_false('make RLC_STOP_ON_LOST_PDU=1 Rel10=1 -j4' + tee, makerr1,  timeout)
        oai.send('cp ./oaisim ./oaisim.rel10.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '08'
        name = 'Compile oai.rel10.itti.make' 
        conf = 'make DISABLE_XER_PRINT=1 ENABLE_ITTI=1 RLC_STOP_ON_LOST_PDU=1 Rel10=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ITTI Rel10'
        oai.send('make cleanall;')
        oai.send('rm -f ./oaisim.rel10.itti.'+host)
        oai.send_expect_false('make DISABLE_XER_PRINT=1 ENABLE_ITTI=1 RLC_STOP_ON_LOST_PDU=1 Rel10=1 -j4' + tee, makerr1,   timeout)
        oai.send('cp ./oaisim ./oaisim.rel10.itti.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '13'
        name = 'Compile oai_nw_ether IP driver' 
        conf = 'make oai_nw_drv'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ITTI Rel8'
        oai.send('make clean;')
        oai.send('make cleanall;')
        oai.send('make cleanasn1;')
        oai.send('rm -f ./oai_nw_drv;')
        oai.send('make oai_nw_drv_clean;')
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('make oai_nw_drv' + tee, makerr1,  60)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    try:
        log.start()
        test = '14'
        name = 'Compile oai.rel8.itti.ral.make' 
        conf = 'make DISABLE_XER_PRINT=1 LINK_ENB_PDCP_TO_IP_DRIVER=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1 ENABLE_RAL=1 MIH_C_MEDIEVAL_EXTENSIONS=1 RLC_STOP_ON_LOST_PDU=1 Rel8=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ITTI Rel8'
        oai.send('make clean;')
        oai.send('make cleanall;')
        oai.send('make cleanasn1;')
        oai.send('rm -f ./oaisim.rel8.itti.ral.'+host)
        oai.send_expect_false('make DISABLE_XER_PRINT=1 LINK_ENB_PDCP_TO_IP_DRIVER=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1 ENABLE_RAL=1 MIH_C_MEDIEVAL_EXTENSIONS=1 RLC_STOP_ON_LOST_PDU=1 Rel8=1 -j4' + tee, makerr1,  timeout)
        oai.send('cp ./oaisim ./oaisim.rel8.itti.ral.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '15'
        name = 'Compile oai.rel10.itti.ral.make' 
        conf = 'make DISABLE_XER_PRINT=1 LINK_ENB_PDCP_TO_IP_DRIVER=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1 ENABLE_RAL=1 MIH_C_MEDIEVAL_EXTENSIONS=1 RLC_STOP_ON_LOST_PDU=1 Rel10=1'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ITTI Rel10'
        oai.send('make clean;')
        oai.send('make cleanall;')
        oai.send('make cleanasn1;')
        oai.send('rm -f ./oaisim.rel10.itti.ral.'+host)
        oai.send_expect_false('make DISABLE_XER_PRINT=1 LINK_ENB_PDCP_TO_IP_DRIVER=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1 ENABLE_RAL=1 MIH_C_MEDIEVAL_EXTENSIONS=1 RLC_STOP_ON_LOST_PDU=1 Rel10=1 -j4' + tee, makerr1,  timeout)
        oai.send('cp ./oaisim ./oaisim.rel10.itti.ral.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    return rv


