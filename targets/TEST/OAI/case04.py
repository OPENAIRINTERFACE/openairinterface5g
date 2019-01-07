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

# \file case04.py
# \brief test case 04 for OAI: executions
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
    
    case = '04'
    oai.send('cd $OPENAIR1_DIR;')     
    oai.send('cd SIMULATION/LTE_PHY;')   
    
    try:
        log.start()
        test = '00'
        name = 'Perf oai.dlsim.sanity'
        conf = '-a -A AWGN -n 100'
        diag = 'dlsim is not running normally (Segmentation fault / Exiting / FATAL), debugging might be needed'
        trace = logdir + '/log_' + host + case + test + '_1.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.' + host + ' ' + conf + tee, 'Segmentation fault', 30)
        trace = logdir + '/log_' + host + case + test + '_2.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.' + host + ' ' + conf + tee, 'Exiting', 30)
        trace = logdir + '/log_' + host + case + test + '_3.txt;'
        tee = ' 2>&1 | tee ' + trace
        oai.send_expect_false('./dlsim.rel8.' + host + ' ' + conf + tee, 'FATAL', 30)

    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
    
    try:
        log.start()
        test = '01'
        name = 'Perf oai.dlsim.test1'
        diag = 'Test 1, 10 MHz, R2.FDD (MCS 5), EVA5, -1dB'
        conf = '-m5 -gF -s-1 -w1.0 -f.2 -n500 -B50 -c2 -z2 -O70 -L'
        trace = logdir + '/log_' + host + case + test +'.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
       
#    try:
#        test = '05'
#        name = 'Perf oai.dlsim.test5'
#        diag = 'Test 5, 1.4 MHz, R4.FDD (MCS 4), EVA5, 0dB (70%)'
#        conf = '-m4 -gF -s0 -w1.0 -f.2 -n500 -B6 -c4 -z2 -O70'
#        trace = logdir + '/log_' + host + case + test + '.txt'
#        tee = ' 2>&1 | tee ' + trace
#        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
#        oai.send_expect(cmd, 'passed', 150)
#    except log.err, e:
#        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
#    else:
#        log.ok(case, test, name, conf, '', logfile)
  
    try:
        log.start()
        test = '06'
        name = 'Perf oai.dlsim.test6'
        diag = 'Test 6, 10 MHz, R3.FDD (MCS 15), EVA5, 6.7dB (70%)'
        conf = '-m15 -gF -s6.7 -w1.0 -f.2 -n500 -B50 -c2 -z2 -O70 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
  
    try:
        log.start()
        test = '06b'
        name = 'Perf oai.dlsim.test6b'
        diag = 'Test 6b, 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (70%)'
        conf = '-m14 -gF -s6.7 -w1.0 -f.2 -n500 -B25 -c3 -z2 -O70 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)
  
    try:
        log.start()
        test = '07'
        name = 'Perf oai.dlsim.test7'
        diag = 'Test 6b, 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (30%)'
        conf = '-m15 -gG -s6.7 -w1.0 -f.2 -n500 -B50 -c2 -z2 -O30 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)


    try:
        log.start()
        test = '07b'
        name = 'Perf oai.dlsim.test7b'
        diag = 'Test 7b, 5 MHz, R3-1.FDD (MCS 15), ETU70, 1.4 dB (30%)'
        conf = '-m14 -gG -s1.4 -w1.0 -f.2 -n500 -B25 -c3 -z2 -O30 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '10'
        name = 'Perf oai.dlsim.test10'
        diag = 'Test 10, 5 MHz, R6.FDD (MCS 25), EVA5, 17.4 dB (70%)'
        conf = '-m25 -gF -s17.4 -w1.0 -f.2 -n500 -B25 -c3 -z2 -O70 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '10b'
        name = 'Perf oai.dlsim.test10b'
        diag = 'Test 10b, 5 MHz, R6-1.FDD (MCS 24,18 PRB), EVA5, 17.5dB (70%)'
        conf = '-m25 -gF -s17.5 -w1.0 -f.2 -n500 -B25 -c3 -z2 -r1022 -O70 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)

    try:
        log.start()
        test = '11'
        name = 'Perf oai.dlsim.test11'
        diag = 'Test 11, 10 MHz, R7.FDD (MCS 25), EVA5, 17.7dB (70%)'
        conf = '-m26 -gF -s17.7 -w1.0 -f.2 -n500 -B50 -c2 -z2 -O70 -L'
        trace = logdir + '/log_' + host + case + test + '.txt'
        tee = ' 2>&1 | tee ' + trace
        cmd = 'taskset -c 0 ./dlsim.rel8.' + host + ' ' + conf + tee
        oai.send_expect(cmd, 'passed', 150)
    except log.err, e:
        log.fail(case, test, name, conf, e.value, diag, logfile,trace)
    else:
        log.ok(case, test, name, conf, '', logfile)


