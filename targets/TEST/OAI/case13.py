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
MCS=[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28]
#MCS=[0,4,9,10,13,16,17,22,27]
#PRB=[100]
#MCS=[16]
ANT_TX=2  # 2 
ANT_RX=2  # 2 
CHANNEL=["N"]
#CHANNEL=["C","E","F","G","H","I","L","M"] # A,B,C,D,E,F,
TX_MODE=2 # 2, 
MIN_SNR=10 
MAX_SNR=40
PERF=75
OPT="-L"
FRAME=2000
#OPT="-L -d" # 8bit decoder , activate dci decoding at UE



def execute(oai, user, pw, host,logfile,logdir,debug,cpu):
    
    case = '10'
    oai.send('cd $OPENAIR_TARGETS;')     
    oai.send('cd bin;')   
    oai.send('cp ./ulsim.Rel10 ./ulsim.Rel10.'+host)
    try:
        log.start()
        test = '300'
        name = 'Run oai.ulsim.sanity'
        conf = '-a -n 100'
        diag = 'ulsim is not running normally (Segmentation fault / Exiting / FATAL), debugging might be needed'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./ulsim.Rel10.'+ host + ' ' + conf + tee, 'Segmentation fault', 30)
        trace = logdir + '/log_' + host + case + test + '_2.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./ulsim.Rel10.'+ host + ' ' + conf + tee, 'Exiting', 30)
        trace = logdir + '/log_' + host + case + test + '_3.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./ulsim.Rel10.'+ host + ' ' + conf + tee, 'FATAL', 30)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    try:
        log.start()
        test = 310
        name = 'Run oai.ulsim.perf.'+str(PERF)+'%'
        diag = 'no diagnostic is available, check the log file'
        for i in range(len(PRB)):
            for o in range(len(CHANNEL)):
                MIN_SNR=10
                for j in range(len(MCS)):
                    for m in range (1,ANT_RX):
                        for p in range(1,TX_MODE):
                              for r in range(5,PRB[i]):
                                  for q in range(MIN_SNR,MAX_SNR): 
                              
                                  
                                    if r ==7 or r ==11 or r ==14 or r == 17 or r==19 or r == 21 or r == 23 or r == 26 or r == 28  : 
                                        continue
                                
                                    conf = '-B' + str(PRB[i]) + ' -r'+str(r) + ' -m'+str(MCS[j]) + ' -y'+str(m) + ' -g'+str(CHANNEL[o]) + ' -x'+str(p) + ' -s'+str(q) + ' -w1.0 -e.1 -P -n'+str(FRAME)+' -O'+str(PERF)+' '+ OPT  
                                    trace = logdir + '/time_meas' + '_prb'+str(PRB[i])+ '_rb'+str(r)+'_mcs'+ str(MCS[j])+ '_antrx' + str(m)  + '_channel' +str(CHANNEL[o]) + '_tx' +str(p) + '_snr' +str(q)+'.'+case+str(test)+ '.log'
                                    tee = ' 2>&1 | tee ' + trace
                                    if cpu > -1 :
                                        cmd = 'taskset -c ' + str(cpu) + ' ./ulsim.Rel10.'+ host + ' ' + conf + tee
                                    else :
                                        cmd = './ulsim.Rel10.'+ host + ' ' + conf + tee
                                        
                                    if debug :
                                        print cmd
                                        
                                    match = oai.send_expect_re(cmd, 'passed', 0, 1000)
                                    #match =1
                                    if match :
                                       
                                        log.ok(case, str(test), name, conf, '', logfile)
                                        MIN_SNR = q - 1 # just to speed up the test
                                        test+=1
                                        break # found the smallest snr
                                    else :
                                        if q == MAX_SNR -1 :
                                            log.skip(case,str(test), name, conf,'','',logfile) 
                                            test+=1
                                            break
                                        try:  
                                            if os.path.isfile(trace) :
                                                os.remove(trace)
                                        except OSError, e:  ## if failed, report it back to the user ##
                                            print ("Error: %s - %s." % (e.filename,e.strerror))
                                            
                                
                                            
    except log.err, e:
        log.fail(case, str(test), name, conf, e.value, diag, logfile,trace)
    #else:
    #    log.ok(case, test, name, conf, '', logfile)
        
