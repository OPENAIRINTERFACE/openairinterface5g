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

# \file core.py
# \brief OAI core testing class that provides various primitives to send/recv cmd to openair class searching for patterns and process the responses and tag the test case as passed/failed/skipped 
# \author Navid Nikaein
# \date 2013
# \version 0.1
# @ingroup _test


import pexpect
import time
import re
import string
import sys
import os

import openair
import log

class core:
    def __init__(self):
        self.send_cr = 1
        self.expect_echo = 0
        self.expect_response = 1
        self.flag_errors = 1
        self.log = None

    def clean(self, obj):
        if type(obj) is str:
            return obj
        else:
            return repr(obj)

    def mark(self, marker):
        if self.log:
            print >> self.log, "\n\n{" + marker + "}\n\n"
            
    def expected(self, expected, got):
        return "================================= Failure ===================================\n"+\
               "_________________________________ Expected __________________________________\n"+\
               "--->" + expected + "<-\n" +\
               "_________________________________ Received __________________________________\n"+\
               "--->" + got + "<-\n" +\
               "=============================================================================\n"

    def unexpected(self, notexpected, got):
        return "================================= Failure ===================================\n"+\
               "__________________________ not expect to find _______________________________\n"+\
               "---> " + self.clean(notexpected) + "\n" +\
               "_________________________________ Received___________________________________\n"+\
               "---> " + self.clean(got) + "\n" +\
               "=============================================================================\n"

    def failed(self, command, expect,debug):
        time.sleep(2)
        ret = "================================= Failure =================================\n"
        ret +="_________________________________ Sent ____________________________________\n"
        ret +="---> " + command + "\n"
        ret +="______________________________Searching for _______________________________\n"
        ret +="---> " + self.clean(expect) + "\n" 
        if debug >= 1 : 
            ret +="________________________________ Received _________________________________\n"
            ret +="---> " + self.clean(self.oai.before) + "\n" 
            ret +="_______________________________ Remaining _________________________________\n"
            ret +="---> " + self.clean(self.oai.after) + "\n" 
        ret +="===========================================================================\n"
        return ret

    def err(self, command):
        return "============================ Error received ================================\n"+\
               "__________________________________ Sent ____________________________________\n"+\
               '---> ' + command + "\n"+\
               "_________________________________ Error was ________________________________\n"+\
               "---> " + self.oai.before + "\n" +\
               "============================================================================\n"

            
    def wait_quiet(self, timeout=0.5):
        while 1:
            try:
                self.oai.expect(['..*'], timeout=0.5)
            except pexpect.TIMEOUT, e:
                return
#            print '[Flushing ' + self.oai.after + ']'

    # **************************Send*****************************    
    # 1) send a command and return, do not wait
    # ************************************************************
    def send_nowait(self, command):
        rsp1 = self.prompt1
        rsp2 = self.prompt2
        self.wait_quiet()
        if self.send_cr:
            log.stats['cmd'] += 1
            self.oai.sendline(command)
        else:
            self.oai.send(command)


    # **************************Send*****************************    
    # 1) send a command
    # 2) wait for a return prompt. Don't capture the response.
    # 3) Check for error or timeout.
    # ************************************************************
    def send(self, command, timeout = 50, rsp1=None, rsp2=None,debug=0):
        if not rsp1:
            rsp1 = self.prompt1
        if not rsp2:
            rsp2 = self.prompt2
        self.wait_quiet()
        if self.send_cr:
            log.stats['cmd'] += 1
            self.oai.sendline(command)
        else:
            self.oai.send(command)

        if self.expect_echo:
            #cmd = self.oai.expect([re.escape(command), pexpect.TIMEOUT], timeout=timeout);
            cmd = self.oai.expect_exact([command, pexpect.TIMEOUT], timeout=timeout);
            if cmd != 0:
                raise log.err(self.failed(command, command,debug))

        if self.expect_response:
            #index = self.oai.expect([re.escape(rsp1), re.escape(rsp2),'%', pexpect.TIMEOUT], timeout=timeout)
            index = self.oai.expect_exact([rsp1, rsp2, pexpect.TIMEOUT], timeout=timeout)

            if index == 0 or index == 1:
                return 'OK'
            elif index == 2:
                #self.oai.expect([re.escape(rsp1), re.escape(rsp2), pexpect.TIMEOUT], timeout=timeout)
                self.oai.expect_exact([rsp1, rsp2, pexpect.TIMEOUT], timeout=timeout)
                if self.flag_errors:
                    raise log.err(self.err(command))
                else:
                    return 'OK'
            else:
                raise log.err(self.failed(command, rsp1 + ' or ' + rsp2,debug))

    # **************************send_recv*************************    
    # 1) send a command
    # 2) wait for either rsp1 or rsp2 is found (normally prompts)
    # 3) return everything seen before that
    # ************************************************************
    def send_recv(self, command, timeout=50, rsp1=None, rsp2=None,debug=0):
        if not rsp1:
            rsp1 = self.prompt1
        if not rsp2:
            rsp2 = self.prompt2
        self.wait_quiet()
        if self.send_cr:
            log.stats['cmd'] += 1
            self.oai.sendline(command)
        else:
            self.oai.send(command)
        #index = self.oai.expect([re.escape(rsp1), re.escape(rsp2), pexpect.TIMEOUT], timeout=timeout);
        index = self.oai.expect_exact([rsp1, rsp2, pexpect.TIMEOUT], timeout=timeout);
        if index == 0 or index == 1 :
            return self.oai.before
        else:
            raise log.err(self.failed(command, rsp1 + ' or ' + rsp2,debug))

           
    # **************************send_expect*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expected pattern in the response
    # 3) raise an error if not found
    # **************************************************************
    def send_expect(self, command, expect, delay = 50, rsp1=None, rsp2=None,debug=0):
        if debug :
            print command 
            print expect 
            print delay
        rsp = self.send_recv(command, delay, rsp1, rsp2)
        #print rsp
        if  (rsp.find(expect) != -1):
            return 'Ok'
        
        raise log.err(self.failed(command, expect,debug))

           
    # **************************send_expect_re*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expected pattern defined by a regular expression in the response
    # 3) return a error if raise_err flag is set and re not found, otherwise return 'Ok'
    # *****************************************************************
    def send_expect_re(self, command, expect, raise_err=1, delay = 50, rsp1=None, rsp2=None,debug=0):
        rsp = self.send_recv(command, delay, rsp1, rsp2)
#        print rsp
        match = re.compile(expect).search(rsp)
        if match:
            return match
        if raise_err:
            raise log.err(self.failed(command, expect,debug))
        else :
            return None
        
    # **************************send_expect*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expected pattern defined by a re in the response
    # 3) return ok if not found
    # **************************************************************
    def send_expect_false(self, command, expect, delay = 5, rsp1=None, rsp2=None,debug=0):
        rsp = self.send_recv(command, delay, rsp1, rsp2)
    #    print rsp
        if  (rsp.find(expect) == -1):
            return 'OK'

        raise log.err(self.failed(command, expect,debug))

    
    # **************************send_wait*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expected pattern in the response
    # 3) retry for a numretries if not found
    # 4) return an error if not found after the numtries
    # 3) return the response if found
    # **************************************************************
    def send_wait(self, command, expect, numretries=3, rsp1=None, rsp2=None,debug=0):
        timer = 0
        for i in range(numretries):
            rsp = self.send_recv(command, 10, rsp1, rsp2)
            if  (rsp.find(expect) != -1):
                return rsp;
            time.sleep(2)
            timer = timer+2
        raise log.err(self.failed(command, expect,debug))
    
    # **************************send_wait_re*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expected pattern defined by a re in the response
    # 3) retry for a numretries if not found
    # 4) return an error if not found after the numtries
    # 3) return the response if found
    # **************************************************************
    def send_wait_re(self, command, expect, numretries=3, rsp1=None, rsp2=None,debug=0):
        timer = 0
        for i in range(numretries):
            rsp = self.send_recv(command)
            if  re.compile(expect).search(rsp):
#                print "Found in",i,"attempts"
                return rsp;
            time.sleep(2)
            timer = timer+2
        raise log.err(self.failed(command, expect,debug))
    
    # **************************send_wait_false*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expect pattern in the response
    # 3) return the response if not found 
    # 4) return an error if the pattern found after the numtries
    # **************************************************************
    def send_wait_false(self, command, expect, numretries=3, rsp1=None, rsp2=None,debug=0):
        timer = 1
        for i in range(numretries):
            rsp = self.send_recv(command)
            if (rsp.find(expect) == -1):
                return rsp;
            time.sleep(2)
            timer = timer+2
        raise log.err(self.failed(command, expect,debug))
    
    # **************************send_wait_false*************************    
    # 1) send a command, and optionally specify a the time to wait
    # 2) search for an expect pattern defined by a re in the response
    # 3) return the response if not found 
    # 4) return an error if the pattern found after the numtries
    # **************************************************************
    def send_wait_false_re(self, command, expect, numretries=3, rsp1=None, rsp2=None,debug=0):
        timer = 0
        for i in range(numretries):
            rsp = self.send_recv(command)
            if  not re.compile(expect).search(rsp):
                return rsp;
            time.sleep(2)
            timer = timer+2
        raise log.err(self.failed(command, expect,debug))
    
    # **************************find*************************    
    # 1) find an exact pattern in a given string 
    # 2) raise an error if not found
    # **************************************************************
    def find(self, string, pattern):
        word = string.replace(pattern,'*','\*')
        words = string.replace(word,' ','\s*')
        if re.search(words,string):
            pass
        else:
            raise log.err(string)
            
    
    # **************************find_false**************************    
    # 1) find an exact pattern in a given string 
    # 2) raise an error if found
    # **************************************************************
    def find_false(self, string, pattern):
        if string.find(pattern) != -1:
            raise log.err(string)
        
    # **************************find_re*************************    
    # 1) find an exact re  pattern in a given string 
    # 2) raise an error if not found
    # **************************************************************
    def find_re(self, string, pattern):
        if not re.compile(pattern).search(string):
            raise log.err(string)
        
    # **************************find_false_re*************************    
    # 1) find an exact re  pattern in a given string 
    # 2) raise an error if found
    # **************************************************************
    def find_false_re(self, string, pattern):
        if re.compile(pattern).search(string):
            raise log.err(string)
    
