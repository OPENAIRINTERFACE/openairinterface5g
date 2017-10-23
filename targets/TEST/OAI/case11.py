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
# \date 2014
# \version 0.1
# @ingroup _test

import log
import openair
import core

makerr1 = '***'
makerr2 = 'Error 1'


def execute(oai, user, pw, host,logfile,logdir,debug):
    
    case = '101'
    rv  = 1; 
    oai.send('cd $OPENAIR1_DIR;')     
    oai.send('cd SIMULATION/LTE_PHY;')   

    try:
        log.start()
        test = '01'
        name = 'Compile oai.rel8.phy.dlsim.make' 
        conf = 'make dlsim'  # PERFECT_CE=1 # for perfect channel estimation
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for dlsim in $OPENAIR1_DIR/SIMULATION/LTE_PHY'
        oai.send('make clean; make cleanall;')
        oai.send('rm -f ./dlsim.rel8.'+host)
        oai.send_expect_false('make dlsim -j4' + tee, makerr1,  1500)
        oai.send('cp ./dlsim ./dlsim.rel8.'+host)
                   
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv =0
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '02'
        name = 'Compile oai.rel8.phy.ulsim.make' 
        conf = 'make ulsim'
        trace = logdir + '/log_' + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'check the compilation errors for ulsim in $OPENAIR1_DIR/SIMULATION/LTE_PHY'
        oai.send('make cleanall;')
        oai.send('rm -f ./ulsim.rel8.'+host)
        oai.send_expect_false('make ulsim -j4' + tee, makerr1,  1500)
        oai.send('cp ./ulsim ./ulsim.rel8.'+host)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
        rv = 0
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    return rv
