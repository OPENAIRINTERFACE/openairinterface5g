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
# \date 2013
# \version 0.1
# @ingroup _test

import time
import random
import log
import openair 
import core
import os


import shutil # copy file 

NUM_UE=1
NUM_eNB=1
NUM_TRIALS=3

PRB=[25,50,100]
MCS=[0,4,9,10,13,16,17,22,27]
#PRB=[100]
#MCS=[27]
#SNR=[0,0,0,0,0,0,0,0,0]
ANT_TX=2  # 2 
ANT_RX=2  # 2 
PDCCH=2 #, 2, 3, 4
CHANNEL=["N"]
#CHANNEL=["C","E","F","G","H","I","L","M"]
TX_MODE=2 # 2, 
MIN_SNR=0
MAX_SNR=40
PERF=75
OPT="-L"
FRAME=2000

#OPT="-L -d" # 8bit decoder , activate dci decoding at UE


def execute(oai, user, pw, host, logfile,logdir,debug,cpu):
    
    case = '10'
    oai.send('cd $OPENAIR1_DIR;')     
    oai.send('cd SIMULATION/LTE_PHY;')   
    try:
        log.start()
        test = '200'
        name = 'Run oai.dlsim.sanity'
        conf = '-a -n 100'
        diag = 'dlsim is not running normally (Segmentation fault / Exiting / FATAL), debugging might be needed'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.'+ host + ' ' + conf + tee, 'Segmentation fault', 30)
        trace = logdir + '/log_'  + host + case + test + '_2.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.'+ host + ' ' + conf + tee, 'Exiting', 30)
        trace = logdir + '/log_'  + host + case + test + '_3.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.'+ host + ' ' + conf + tee, 'FATAL', 30)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    try:
        log.start()
        test = 210
       
        name = 'Run oai.dlsim.perf.'+str(PERF)+'%'
        diag = 'no diagnostic is available, check the log file'
        for i in range(len(PRB)):
            for o in range(len(CHANNEL)):
                MIN_SNR = 0
                for j in range(len(MCS)):
                    for k in range(1,ANT_TX):
                        for m in range (1,ANT_RX):
                            for n in range(1,PDCCH):
                                for p in range(1,TX_MODE):
                                    for q in range(MIN_SNR,MAX_SNR): 
                                        #if  if PRB[i] :
                                                                          
                                        conf = '-B' + str(PRB[i]) + ' -m'+str(MCS[j]) + ' -y'+str(k) + ' -z'+str(m) +' -c'+str(n) + ' -g'+str(CHANNEL[o]) + ' -x'+str(p) + ' -s'+str(q) + ' -w1.0 -f.1 -P -n'+str(FRAME)+' -O'+str(PERF) +' '+ OPT    
                                        trace = logdir + '/time_meas' + '_prb'+str(PRB[i])+'_mcs'+ str(MCS[j])+ '_anttx' + str(k)+ '_antrx' + str(m)  + '_pdcch' + str(n) + '_channel' +str(CHANNEL[o]) + '_tx' +str(p) + '_snr' +str(q)+'.'+case+str(test)+ '.log'
                                        tee = ' 2>&1 | tee ' + trace
                                        if cpu > -1 : 
                                            cmd = 'taskset -c '+ str(cpu) + ' ./dlsim.rel8.'+ host + ' ' + conf + tee
                                        else :    
                                            cmd = './dlsim.rel8.'+ host + ' ' + conf + tee
                                        
                                        if debug : 
                                            print cmd

                                        match = oai.send_expect_re(cmd, 'passed', 0, 1000)
                                      
                                        if match :
                                            #SNR[j]=q
                                            log.ok(case, str(test), name, conf, '', logfile)
                                            MIN_SNR = q -1 # just to speed up the test
                                            test+=1
                                            break # found the smallest snr
                                        else :
                                            if q == MAX_SNR -1 :
                                                log.skip(case,str(test), name, conf,'','',logfile) 
                                                test+=1
                                                break # do not remove the last log file
                                            try:  
                                                if os.path.isfile(trace) :
                                                    os.remove(trace)
                                             
                                            except OSError, e:  ## if failed, report it back to the user ##
                                                print ("Error: %s - %s." % (e.filename,e.strerror))
                                        
                                       
                                                
    except log.err, e:
        log.fail(case, str(test), name, conf, e.value, diag, logfile,trace)
    #else:
    #    log.ok(case, test, name, conf, '', logfile)
        
