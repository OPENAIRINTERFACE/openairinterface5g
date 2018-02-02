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

# \file case03.py
# \brief test case 03 for OAI: executions
# \author Navid Nikaein
# \date 2013
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
    
    case = '03'
    oai.send('cd $OPENAIR_TARGETS;')
    oai.send('cd SIMU/USER;')
    
    try:
        log.start()
        test = '00'
        name = 'Run oai.rel10.sf'
        conf = '-a -A AWGN -l7 -n 100'
        diag = 'OAI is not running normally (Segmentation fault / Exiting / FATAL), debugging might be needed'
        trace = logdir + '/log_' + host + case + test + '_1.txt'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel10.' + host + ' ' + conf + tee, 'Segmentation fault', 30)
        trace = logdir + '/log_' + host + case + test + '_2.txt'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel10.' + host + ' ' + conf + tee, 'Exiting', 30)
        trace = logdir + '/log_' + host + case + test + '_3.txt'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./oaisim.rel10.' + host + ' ' + conf + tee, 'FATAL', 30)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '01'
        name = 'Run oai.rel10.err'
        conf = '-a -A AWGN -l7 -n 100'
        trace = logdir + '/log_' + host + case + test + '.txt;'
        tee = ' 2>&1 | tee ' + trace
        diag = 'Error(s) found in the execution, check the execution logs'
        oai.send_expect_false('./oaisim.rel10.' + host + ' ' + conf + tee, '[E]', 30)
        
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
        
    try:
        log.start()
        test = '02'
        name = 'Run oai.rel10.tdd.5MHz.abs.rrc'
        diag = 'RRC procedure is not finished completely, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-a -l7 -A AWGN -n' + str((i+1+j) * 50) + ' -u' + str(i+1) +' -b'+ str(j+1)
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 100)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)    

    try:
        log.start()
        test = '03'
        name = 'Run oai.rel10.tdd.5MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-A AWGN  -l7 -x 1 -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1)
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)  

    try:
        log.start()
        test = '04'
        name = 'Run oai.rel10.fdd.5MHz.phy.rrc'
        diag = 'RRC procedure is not finished completely in FDD mode, check the execution logs and trace BCCH, CCCH, and DCCH channels'
        for i in range(NUM_UE) :
            for j in range(NUM_eNB) :
                conf = '-A AWGN -l7 -F -x 1 -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1)
                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt;'
                tee = ' 2>&1 | tee ' + trace
                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Received RRCConnectionReconfigurationComplete from UE ' + str(i),  (i+1) * 500)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)  


 #   try:
 #       test = '05'
#        name = 'Run oai.rel10.phy.eMBMS.MCCH'
#        diag = 'eMBMS procedure is not finished completely, make sure that the SIB13/MCCH have been correclty received by UEs'
#        for i in range(NUM_UE) :
#            for j in range(NUM_eNB) :
#                conf = '-A AWGN -l7 -x 1 -Q3 -n' + str((i+1+j) * 50) + ' -u' + str(i+1) +' -b'+ str(j+1)
#                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt'
#                tee = ' 2>&1 | tee ' + trace
#                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Found MBSFNAreaConfiguration from eNB ' + str(j),  (i+1) * 200)
#    except log.err, e:
#        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
#    else:
#        log.ok(case, test, name, conf, '', logfile)  
        
#    try:
#        test = '06'
#        name = 'Run oai.rel10.phy.eMBMS.OTG'
#        diag = 'eMBMS multicast/broadcast data is not received, make sure that the SIB13/MCCH/MTCH have been correclty received by UEs'
#        for i in range(NUM_UE) :
#            for j in range(NUM_eNB) :
#                conf = '-A AWGN -l7 -x 1 -T mscbr -Q3 -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1)
#                trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt'
#                tee = ' 2>&1 | tee ' + trace
#                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Received a multicast packet',  (i+1) * 200)
#    except log.err, e:
#        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
#    else:
#        log.ok(case, test, name, conf, 'Note: check the packet loss from the OTG stats', logfile)   

#    try:
#        test = '07'
#        name = 'Run oai.rel10.phy.eMBMS.OTG.fdd'
#        diag = 'eMBMS multicast/broadcast data is not received in fdd mode, make sure that the SIB13/MCCH/MTCH have been correclty received by UEs'
#        for i in range(NUM_UE) :
#            for j in range(NUM_eNB) :
#                conf = '-A AWGN -l7 -F -x 1 -T mscbr -Q3 -n' + str((i+1+j) * 100) + ' -u' + str(i+1) +' -b'+ str(j+1)
#               trace = logdir + '/log_' + host + case + test + '_' + str(i) + str(j) + '.txt'
#               tee = ' 2>&1 | tee ' + trace
#                oai.send_expect('./oaisim.rel10.' + host + ' ' + conf + tee, ' Received a multicast packet',  (i+1) * 200)
#    except log.err, e:
#        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
#    else:
#        log.ok(case, test, name, conf, 'Note: check the packet loss from the OTG stats', logfile)   

   # try:
   #     test = '08'
   #     name = 'Run oai.rel10.phy.eMBMS.Relay.OTG.fdd'
   #     diag = 'eMBMS multicast/broadcast DF relaying is not working properly in fdd mode, make sure that the SIB13/MCCH/MTCH have been correclty received by UEs'
   #     conf = '-c43 -F -T mbvbr -Q4 -j1 -n120' 
   #     tee = ' | tee ' + logs_dir + '/log_' + case + test + '.txt'
   #     oai.send_expect('./oaisim.rel10 ' + conf + tee, ' MTCH for sync area 1', 100)
   # except log.err, e:
   #     log.fail(case, test, name, conf, e.value, diag, logfile)
   # else:
   #     log.ok(case, test, name, conf, 'Note: check the packet loss from the OTG stats', logfile)   

#    try:
#        test = '09'
#        name = 'Run oai.rel10.itti.phy.eMBMS.MCCH'
#        diag = 'eMBMS procedure is not finished completely, check the eNB config file (enb.band7.generic.conf), and make sure that the SIB13/MCCH have been correclty received by UEs'
#        for i in range(NUM_UE) :
#            for j in range(NUM_eNB) :
#                log_name = logdir + '/log_' + host + case + test + '_' + str(i) + str(j)
#                itti_name = log_name + '.log'
#                trace_name = log_name + '.txt'
#                conf = '-A AWGN -l7 -x 1 -Q3 --enb-conf ../../PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.conf -n' + str((i+1+j) * 50) + ' -u' + str(i+1) +' -b'+ str(j+1) + ' -K' + itti_name
#                tee = ' 2>&1 | tee -a ' + trace_name
#                command = './oaisim.rel10.itti.' + host + ' ' + conf
#                oai.send('echo ' + command + ' > ' + trace_name + ';')
 #               oai.send_expect(command + tee, ' Found MBSFNAreaConfiguration from eNB ' + str(j),  (i+1) * 200)
#    except log.err, e:
#        log.fail(case, test, name, conf, e.value, diag, logfile, trace_name)
#    else:
#        log.ok(case, test, name, conf, '', logfile)  
        



