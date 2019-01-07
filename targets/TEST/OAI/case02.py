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

# \file case02.py
# \brief test case 02 for OAI: executions
# \author Navid Nikaein
# \date 2013 - 2015
# \version 0.1
# @ingroup _test

import time
import random
import log
import openair 
import core

NUM_UE=2
NUM_eNB=1
NUM_TRIALS=3

def execute(oai, user, pw, host, logfile,logdir,debug):
    
    case = '02'
    oai.send('cd $OPENAIR_TARGETS;')
    oai.send('cd SIMU/USER;')
    
    try:
        log.start()
        test = '00'
        name = 'Run oai.rel8.sf'
        conf = '-a -A AWGN -n 100'
        diag = 'OAI is not running normally (Segmentation fault / Exiting / FATAL), debugging might be needed'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel8.' + host + ' ' + conf + tee, 'Segmentation fault', 30)
        trace = logdir + '/log_' + host + case + test + '_2.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel8.' + host + ' ' + conf + tee, 'Exiting', 30)
        trace = logdir + '/log_' + host + case + test + '_3.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel8.' + host + ' ' + conf + tee, 'FATAL', 30)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '01'
        name = 'Run oai.rel8.err'
        conf = '-a -A AWGN -n 100 '
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = '[E] Error(s) found during the execution, check the execution logs'
        oai.send_expect_false('./oaisim.rel8.'+ host + ' ' + conf, '[E]', 30)
        
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
        
    try:
        log.start()
        test = '02'
        name = 'Run oai.rel8.tdd.5MHz.rrc.abs'
        diag = 'RRC procedure is not finished completely, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-a -A AWGN -n' + str((i+1+j) * 50) + ' -u' + str(i+1) +' -b'+ str(j+1)
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 100)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
        
    try:
        log.start()
        test = '03'
        name = 'Run oai.rel8.tdd.5MHz.rrc.itti.abs'
        diag = 'RRC procedure is not finished completely, check the eNB config file (default is enb.band7.generic.conf), in addition to the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                log_name = logdir + '/log_' + host + case + test + '_' + str(i) + str(j)
                itti_name = log_name + '.log'
                trace_name = log_name + '.txt'
                conf = '-a -l7 -A AWGN --enb-conf ../../PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.conf -n' + str((i+1+j) * 50) + ' -u' + str(i+1) +' -b'+ str(j+1) + ' -K' + itti_name
                tee = ' 2>&1 | tee ' + trace_name
                command = './oaisim.rel8.itti.' + host + ' ' + conf
                oai.send('echo ' + command + ' > ' + trace_name + ';')
                oai.send_expect(command + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile, trace_name)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    try:
        log.start()
        test='04'
        name = 'Run oai.rel8.tdd.5MHz.abs.ocg.otg'
        diag = 'Check the scenario if the test 0202 is passed.'
        conf = '-a -c26'
        trace = logdir + '/log_' + host + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' DL and UL loss rate below 10 ', 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test='05'
        name = 'Run oai.rel8.fdd.5MHz.abs.ocg.otg'
        diag = 'Check the template 26 and the results of test 0202.'
        conf = '-a -F -c26'
        trace = logdir + '/log_' + host + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' DL and UL loss rate below 10 ', 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
    

    try:
        log.start()
        test = '06'
        name = 'Run oai.rel8.tdd.5MHz.abs.ping'
        diag = 'Data-plane is not working normally, check the OAI protocol stack, OAI driver, and normal operation of the OS'
        
        oai.driver(oai,user,pw)

        for i in range(NUM_eNB) :
            for j in range(NUM_UE) :
                conf = '-a -A AWGN  -u' + str(j+1) +' -b'+ str(i+1)
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 > ' + trace

                if user == 'root' :
                    oai.send_nowait('./oaisim.rel8.nas.' + host + ' ' + conf + ' &')
                else :    
                    oai.send_nowait('echo '+pw+ ' | sudo -S -E ./oaisim.rel8.nas.'+ host + ' ' + conf + tee + ' &')
                time.sleep(10)
                for k in range(NUM_TRIALS) :
                    trace_ping = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + str(k) + '_ping.txt;'
                    tee_ping = ' 2>&1 | tee ' + trace_ping

                    oai.send_expect('ping 10.0.'+str(j+1)+'.'+str(NUM_eNB+i+1) + ' -c ' +  str(random.randint(2, 10))+ ' -s ' + str(random.randint(128, 1500)) + tee_ping, ' 0% packet loss', 20)
                if user == 'root' :
                    oai.send('pkill -f oaisim.rel8.nas.'+host)
                    time.sleep(1)
                    oai.send('pkill -f oaisim.rel8.nas.'+host)
                else :
                    oai.send('pkill -f oaisim.rel8.nas.'+host)
                    time.sleep(1)
                    oai.send('echo '+pw+ ' | sudo -S pkill -f oaisim.rel8.nas.'+host)
                    time.sleep(1)
                    oai.send('echo '+pw+ ' | sudo -S pkill -f oaisim.rel8.nas.'+host)
                    time.sleep(1)

        oai.rm_driver(oai,user,pw)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '07'
        name = 'Run oai.rel8.tdd.5MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-A AWGN -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1) + ' -x1'
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '08'
        name = 'Run oai.rel8.fdd.5MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely in FDD mode, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-A AWGN -F -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1) + ' -x1'
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i), (i+1) * 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '09'
        name = 'Run oai.rel8.fdd.10MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely, check th execution logs and trace BCCH, CCCH, and DCCH channels and the results of test 0204'
        conf = '-A AWGN -F -R 50 -n 150 -u 1 -b 1 -x1'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE 0', 600)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '10'
        name = 'Run oai.rel8.fdd.20MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely, check th execution logs and trace BCCH, CCCH, and DCCH channels and the results of test 0204'
        conf = '-A AWGN -F -R 100 -n 200 -u 1 -b 1 -x1'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect('./oaisim.rel8.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE 0', 700)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)    
