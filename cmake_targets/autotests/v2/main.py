#!/usr/bin/python

import os, re, sys, time, threading, thread
import xml.etree.ElementTree as ET

from utils import test_in_list, quickshell, log
from task import Task, WAITLOG_SUCCESS, WAITLOG_FAILURE
from machine_list import MachineList

oai_user         = os.environ.get('OAI_USER')
oai_password     = os.environ.get('OAI_PASS')
requested_tests  = os.environ.get('OAI_TEST_CASE_GROUP')
machines         = os.environ.get('MACHINELIST')
generic_machines = os.environ.get('MACHINELISTGENERIC')
result_dir       = os.environ.get('RESULT_DIR')
nruns_softmodem  = os.environ.get('NRUNS_LTE_SOFTMODEM')
openair_dir      = os.environ.get('OPENAIR_DIR')

some_undef = False
if (oai_user         == None) :
        log("variable OAI_USER is not defined")
        some_undef = True
if (oai_password     == None) :
        log("variable OAI_PASS is not defined")
        some_undef = True
if (requested_tests  == None) :
        log("variable OAI_TEST_CASE_GROUP is not defined")
        some_undef = True
if (machines         == None) :
        log("variable MACHINELIST is not defined")
        some_undef = True
if (generic_machines == None) :
        log("variable MACHINELISTGENERIC is not defined")
        some_undef = True
if (result_dir       == None) :
        log("variable RESULT_DIR is not defined")
        some_undef = True
if (nruns_softmodem  == None) :
        log("variable NRUNS_LTE_SOFTMODEM is not defined")
        some_undef = True
if (openair_dir      == None) :
        log("variable OPENAIR_DIR is not defined")
        some_undef = True
if (some_undef == True):
    os._exit(1)

requested_tests  = requested_tests  .replace('"','')
machines         = machines         .replace('"','')
generic_machines = generic_machines .replace('"','')

xml_test_file = os.environ.get('OPENAIR_DIR') + \
                "/cmake_targets/autotests/test_case_list.xml"

xmlTree = ET.parse(xml_test_file)
xmlRoot = xmlTree.getroot()

exclusion_tests=xmlRoot.findtext('TestCaseExclusionList',default='')
all_tests=xmlRoot.findall('testCase')

exclusion_tests=exclusion_tests.split()
requested_tests=requested_tests.split()

#check that exclusion tests are well formatted
#(6 digits or less than 6 digits followed by +)
for test in exclusion_tests:
    if     (not re.match('^[0-9]{6}$', test) and
            not re.match('^[0-9]{1,5}\+$', test)):
        log("ERROR: exclusion test is invalidly formatted: " + test)
        os._exit(1)

#check that requested tests are well formatted
#(6 digits or less than 6 digits followed by +)
#be verbose
for test in requested_tests:
    if     (re.match('^[0-9]{6}$', test) or
            re.match('^[0-9]{1,5}\+$', test)):
        log("INFO: test group/case requested: " + test)
    else:
        log("ERROR: requested test is invalidly formatted: " + test)
        os._exit(1)

#get the list of tests to be done
todo_tests=[]
for test in all_tests:
    if     (test_in_list(test.get('id'), requested_tests) and
            not test_in_list(test.get('id'), exclusion_tests)):
        log("INFO: test will be run: " + test.get('id'))
        todo_tests.append(test)
    else:
        log("INFO: test will be skipped: " + test.get('id'))

#get commit ID to use
commit_id = quickshell("git rev-parse --verify HEAD").replace('\n','')
if (len(commit_id) != 20*2):
    log("ERROR: bad commit '" + commit_id + "'")
log("INFO: test for commit " + commit_id)

#get repository URL
repository_url = quickshell("git config remote.origin.url").replace('\n','')
log("INFO: repository URL: " + repository_url)

#prepare environment for tasks
env = []
env.append("REPOSITORY_URL=" + repository_url)
env.append("COMMIT_ID="      + commit_id)

#clone repository on all machines in the test setup
tasks=[]
for machine in machines.split():
    tasks.append(Task("actions/clone_repository.bash",
                      "clone repository on " + machine,
                      machine,
                      oai_user,
                      oai_password,
                      env,
                      openair_dir + "/cmake_targets/autotests/log/clone." \
                         + machine))
for task in tasks:
    log("INFO: wait for task: " + task.description)
    ret = task.wait()
    if ret != 0 or not "TEST_SETUP_SUCCESS" in open(task.logfile).read():
        log("ERROR: task failed: " + task.description)
        os._exit(1)

##############################################################################
# run compilation tests                                                      #
##############################################################################

machine_list = MachineList(generic_machines.split())

for test in todo_tests:
    action = test.findtext('class')
    if action != 'compilation':
        continue
    id = test.get('id')
    machine = machine_list.get_free_machine()
    log("INFO: start " + action + " test " + id + " on machine " +
        machine.name)
    tasks = []
    runenv = list(env)
    runenv.append('OPENAIR_DIR=/tmp/oai_test_setup/oai')
    runenv.append('BUILD_ARGUMENTS="'
                    + test.findtext('compile_prog_args')
                    + '"')
    runenv.append('BUILD_OUTPUT="'
                    + test.findtext('compile_prog_out')
                          .replace('\n','')
                    + '"')
    logdir = openair_dir +"/cmake_targets/autotests/log/"+ id +"/compile_log"
    remote_files = "'/tmp/oai_test_setup/oai/cmake_targets/log/*'"
    post_action = "mkdir -p "+ logdir + \
                  " && sshpass -p " + oai_password + " scp -r " + oai_user + \
                  "@" + machine.name + ":" + remote_files + " " + logdir + \
                  " || true"
    tasks.append(Task("actions/" + action + ".bash",
                      action + " of test " + id + " on " + machine.name,
                      machine.name,
                      oai_user,
                      oai_password,
                      runenv,
                      openair_dir + "/cmake_targets/autotests/log/"
                         + id + "."
                         + machine.name,
                      post_action=post_action))
    machine.busy(tasks)

##############################################################################
# run execution tests                                                        #
##############################################################################

class ExecutionThread(threading.Thread):
    def __init__(self, id, machine, test):
        threading.Thread.__init__(self)
        self.machine = machine
        self.id = id
        self.test = test

    def run(self):
        id = self.id
        machine = self.machine
        test = self.test

        # step 1: compile

        log("INFO: start compilation of test " + id + " on machine " +
            machine.name)
        tasks = []
        runenv = list(env)
        runenv.append('OPENAIR_DIR=/tmp/oai_test_setup/oai')
        runenv.append('PRE_BUILD="'
                        + test.findtext('pre_compile_prog')
                        + '"')
        runenv.append('BUILD_PROG="'
                        + test.findtext('compile_prog')
                        + '"')
        runenv.append('BUILD_ARGUMENTS="'
                        + test.findtext('compile_prog_args')
                        + '"')
        runenv.append('PRE_EXEC="'
                        + test.findtext('pre_exec') + " "
                        + test.findtext('pre_exec_args')
                        + '"')
        logdir = openair_dir +"/cmake_targets/autotests/log/"+ id + \
                 "/compile_log"
        remote_files = "'/tmp/oai_test_setup/oai/cmake_targets/log/*'"
        post_action = "mkdir -p "+ logdir + \
                      " && sshpass -p " + oai_password + \
                      " scp -r " + oai_user + "@" + machine.name + ":" + \
                                   remote_files + " " + logdir + \
                      " || true"
        task = Task("actions/execution_compile.bash",
                    "compilation of test " + id + " on " + machine.name,
                    machine.name,
                    oai_user,
                    oai_password,
                    runenv,
                    openair_dir + "/cmake_targets/autotests/log/"
                       + id + "_compile."
                       + machine.name,
                    post_action=post_action)
        ret = task.wait()
        task.postaction()
        if ret != 0:
            log("ERROR: compilation of test " + id + " failed: " + str(ret))
            machine.unbusy()
            return

        #step 2: run all tests

        nruns = test.findtext('nruns')
        args = test.findtext('main_exec_args')
        i = 0
        for arg in args.splitlines():
            i = i+1
            runenv2 = list(runenv)
            runenv2.append('OPENAIR_TARGETS=/tmp/oai_test_setup/oai/targets')
            runenv2.append('EXEC="'
                             + test.findtext('main_exec')
                             + '"')
            runenv2.append('EXEC_ARGS="' + arg + '"')
            for run in range(int(nruns)):
                log("INFO: start execution of test " + id + " case " +
                    str(i) + " run " + str(run) + " on machine " +
                    machine.name)
                task =Task("actions/execution.bash",
                           "execution of test " + id + " on " + machine.name,
                           machine.name,
                           oai_user,
                           oai_password,
                           runenv2,
                           openair_dir + "/cmake_targets/autotests/log/"
                              + id + "_case_" + str(i) + "_run_" + str(run)
                              + "." + machine.name)
                ret = task.wait()
                if ret != 0:
                    log("ERROR: execution of test " +id+ " failed: " +
                        str(ret))

        machine.unbusy()

for test in todo_tests:
    action = test.findtext('class')
    if action != 'execution':
        continue
    id = test.get('id')
    machine = machine_list.get_free_machine()
    ExecutionThread(id, machine, test).start()

#wait for compilation/execution tests to be finished
#log only if some compilation/execution tests are actually done
for test in todo_tests:
    action = test.findtext('class')
    if action == 'execution' or action == 'compilation':
        log("INFO: requested compilation/execution tests " +
            "have been launched, waiting for completion")
        break
machine_list.wait_all_free()

##############################################################################
# run eNB softmodem tests                                                    #
##############################################################################

tests = {}
for a in { 'b210', 'remote b210', 'x310', 'exmimo2' }:
  tests[a] = {}
  for b in { 'alu', 'openair-cn' }:
    tests[a][b] = {}
    for c in { 'fdd', 'tdd' }:
      tests[a][b][c] = {}
      for d in { '5', '10', '20' }:
        tests[a][b][c][d] = {}
        for e in { 'bandrich', 'sony', '3276' }:
          tests[a][b][c][d][e] = {}
          for f in { 'tcp', 'udp' }:
            tests[a][b][c][d][e][f] = {}
            for g in { 'dl', 'ul' }:
              tests[a][b][c][d][e][f][g] = False

todo_tests_ids = []
for test in todo_tests:
    todo_tests_ids.append(test.get('id'))

for test in todo_tests_ids:
  if test=='015500':tests['b210']['alu']['fdd'][ '5']['bandrich']['udp']['ul']=True
  if test=='015501':tests['b210']['alu']['fdd']['10']['bandrich']['udp']['ul']=True
  if test=='015502':tests['b210']['alu']['fdd']['20']['bandrich']['udp']['ul']=True
  if test=='015503':tests['b210']['alu']['fdd'][ '5']['bandrich']['udp']['dl']=True
  if test=='015504':tests['b210']['alu']['fdd']['10']['bandrich']['udp']['dl']=True
  if test=='015505':tests['b210']['alu']['fdd']['20']['bandrich']['udp']['dl']=True
  if test=='015506':log('WARNING: skip test ' + test) #TODO
  if test=='015507':log('WARNING: skip test ' + test) #TODO
  if test=='015508':log('WARNING: skip test ' + test) #TODO
  if test=='015509':log('WARNING: skip test ' + test) #TODO
  if test=='015510':log('WARNING: skip test ' + test) #TODO
  if test=='015511':log('WARNING: skip test ' + test) #TODO
  if test=='015512':tests['b210']['alu']['fdd'][ '5']['bandrich']['tcp']['ul']=True
  if test=='015513':tests['b210']['alu']['fdd']['10']['bandrich']['tcp']['ul']=True
  if test=='015514':tests['b210']['alu']['fdd']['20']['bandrich']['tcp']['ul']=True
  if test=='015515':tests['b210']['alu']['fdd'][ '5']['bandrich']['tcp']['dl']=True
  if test=='015516':tests['b210']['alu']['fdd']['10']['bandrich']['tcp']['dl']=True
  if test=='015517':tests['b210']['alu']['fdd']['20']['bandrich']['tcp']['dl']=True
  if test=='015518':log('WARNING: skip test ' + test) #TODO
  if test=='015519':log('WARNING: skip test ' + test) #TODO
  if test=='015520':log('WARNING: skip test ' + test) #TODO
  if test=='015521':log('WARNING: skip test ' + test) #TODO
  if test=='015522':log('WARNING: skip test ' + test) #TODO
  if test=='015523':log('WARNING: skip test ' + test) #TODO

  if test=='015600':log('WARNING: skip test ' + test) #TODO
  if test=='015601':log('WARNING: skip test ' + test) #TODO
  if test=='015602':log('WARNING: skip test ' + test) #TODO
  if test=='015603':log('WARNING: skip test ' + test) #TODO
  if test=='015604':log('WARNING: skip test ' + test) #TODO
  if test=='015605':log('WARNING: skip test ' + test) #TODO

  if test=='015700':log('WARNING: skip test ' + test) #TODO
  if test=='015701':log('WARNING: skip test ' + test) #TODO
  if test=='015702':log('WARNING: skip test ' + test) #TODO
  if test=='015703':log('WARNING: skip test ' + test) #TODO
  if test=='015704':log('WARNING: skip test ' + test) #TODO
  if test=='015705':log('WARNING: skip test ' + test) #TODO

  if test=='015800':log('WARNING: skip test ' + test) #TODO
  if test=='015801':log('WARNING: skip test ' + test) #TODO
  if test=='015802':log('WARNING: skip test ' + test) #TODO
  if test=='015803':log('WARNING: skip test ' + test) #TODO
  if test=='015804':log('WARNING: skip test ' + test) #TODO
  if test=='015805':log('WARNING: skip test ' + test) #TODO
  if test=='015806':log('WARNING: skip test ' + test) #TODO
  if test=='015807':log('WARNING: skip test ' + test) #TODO
  if test=='015808':log('WARNING: skip test ' + test) #TODO
  if test=='015809':log('WARNING: skip test ' + test) #TODO
  if test=='015810':log('WARNING: skip test ' + test) #TODO
  if test=='015811':log('WARNING: skip test ' + test) #TODO
  if test=='015812':log('WARNING: skip test ' + test) #TODO
  if test=='015813':log('WARNING: skip test ' + test) #TODO
  if test=='015814':log('WARNING: skip test ' + test) #TODO
  if test=='015815':log('WARNING: skip test ' + test) #TODO
  if test=='015816':log('WARNING: skip test ' + test) #TODO
  if test=='015817':log('WARNING: skip test ' + test) #TODO
  if test=='015818':log('WARNING: skip test ' + test) #TODO
  if test=='015819':log('WARNING: skip test ' + test) #TODO
  if test=='015820':log('WARNING: skip test ' + test) #TODO
  if test=='015821':log('WARNING: skip test ' + test) #TODO
  if test=='015822':log('WARNING: skip test ' + test) #TODO
  if test=='015823':log('WARNING: skip test ' + test) #TODO

  if test=='016000':log('WARNING: skip test ' + test) #TODO
  if test=='016001':log('WARNING: skip test ' + test) #TODO
  if test=='016002':log('WARNING: skip test ' + test) #TODO
  if test=='016003':log('WARNING: skip test ' + test) #TODO
  if test=='016004':log('WARNING: skip test ' + test) #TODO
  if test=='016005':log('WARNING: skip test ' + test) #TODO

  if test=='016100':log('WARNING: skip test ' + test) #TODO
  if test=='016101':log('WARNING: skip test ' + test) #TODO
  if test=='016102':log('WARNING: skip test ' + test) #TODO
  if test=='016103':log('WARNING: skip test ' + test) #TODO
  if test=='016104':log('WARNING: skip test ' + test) #TODO
  if test=='016105':log('WARNING: skip test ' + test) #TODO

  if test=='016300':log('WARNING: skip test ' + test) #TODO
  if test=='016301':log('WARNING: skip test ' + test) #TODO
  if test=='016302':log('WARNING: skip test ' + test) #TODO
  if test=='016303':log('WARNING: skip test ' + test) #TODO
  if test=='016304':log('WARNING: skip test ' + test) #TODO
  if test=='016305':log('WARNING: skip test ' + test) #TODO

  if test=='016500':log('WARNING: skip test ' + test) #TODO
  if test=='016501':log('WARNING: skip test ' + test) #TODO
  if test=='016502':log('WARNING: skip test ' + test) #TODO
  if test=='016503':log('WARNING: skip test ' + test) #TODO
  if test=='016504':log('WARNING: skip test ' + test) #TODO
  if test=='016505':log('WARNING: skip test ' + test) #TODO

  if test=='017000':log('WARNING: skip test ' + test) #TODO
  if test=='017001':log('WARNING: skip test ' + test) #TODO
  if test=='017002':log('WARNING: skip test ' + test) #TODO
  if test=='017003':log('WARNING: skip test ' + test) #TODO
  if test=='017004':log('WARNING: skip test ' + test) #TODO
  if test=='017005':log('WARNING: skip test ' + test) #TODO

  if test=='017500':log('WARNING: skip test ' + test) #TODO
  if test=='017501':log('WARNING: skip test ' + test) #TODO
  if test=='017502':log('WARNING: skip test ' + test) #TODO
  if test=='017503':log('WARNING: skip test ' + test) #TODO
  if test=='017504':log('WARNING: skip test ' + test) #TODO
  if test=='017505':log('WARNING: skip test ' + test) #TODO

  if test=='017600':tests['remote b210']['alu']['fdd'][ '5']['bandrich']['udp']['ul']=True
  if test=='017601':tests['remote b210']['alu']['fdd']['10']['bandrich']['udp']['ul']=True
  if test=='017602':tests['remote b210']['alu']['fdd']['20']['bandrich']['udp']['ul']=True
  if test=='017603':tests['remote b210']['alu']['fdd'][ '5']['bandrich']['udp']['dl']=True
  if test=='017604':tests['remote b210']['alu']['fdd']['10']['bandrich']['udp']['dl']=True
  if test=='017605':tests['remote b210']['alu']['fdd']['20']['bandrich']['udp']['dl']=True
  if test=='017606':tests['remote b210']['alu']['fdd'][ '5']['bandrich']['tcp']['ul']=True
  if test=='017607':tests['remote b210']['alu']['fdd']['10']['bandrich']['tcp']['ul']=True
  if test=='017608':tests['remote b210']['alu']['fdd']['20']['bandrich']['tcp']['ul']=True
  if test=='017609':tests['remote b210']['alu']['fdd'][ '5']['bandrich']['tcp']['dl']=True
  if test=='017610':tests['remote b210']['alu']['fdd']['10']['bandrich']['tcp']['dl']=True
  if test=='017611':tests['remote b210']['alu']['fdd']['20']['bandrich']['tcp']['dl']=True

  if test=='018000':log('WARNING: skip test ' + test) #TODO
  if test=='018001':log('WARNING: skip test ' + test) #TODO
  if test=='018002':log('WARNING: skip test ' + test) #TODO
  if test=='018003':log('WARNING: skip test ' + test) #TODO
  if test=='018004':log('WARNING: skip test ' + test) #TODO
  if test=='018005':log('WARNING: skip test ' + test) #TODO

  if test=='018500':log('WARNING: skip test ' + test) #TODO
  if test=='018501':log('WARNING: skip test ' + test) #TODO
  if test=='018502':log('WARNING: skip test ' + test) #TODO
  if test=='018503':log('WARNING: skip test ' + test) #TODO
  if test=='018504':log('WARNING: skip test ' + test) #TODO
  if test=='018505':log('WARNING: skip test ' + test) #TODO

  if test=='018600':tests['b210']['alu']['tdd'][ '5']['3276']['udp']['ul']=True
  if test=='018601':tests['b210']['alu']['tdd']['10']['3276']['udp']['ul']=True
  if test=='018602':tests['b210']['alu']['tdd']['20']['3276']['udp']['ul']=True
  if test=='018603':tests['b210']['alu']['tdd'][ '5']['3276']['udp']['dl']=True
  if test=='018604':tests['b210']['alu']['tdd']['10']['3276']['udp']['dl']=True
  if test=='018605':tests['b210']['alu']['tdd']['20']['3276']['udp']['dl']=True
  if test=='018606':log('WARNING: skip test ' + test) #TODO
  if test=='018607':log('WARNING: skip test ' + test) #TODO
  if test=='018608':log('WARNING: skip test ' + test) #TODO
  if test=='018609':log('WARNING: skip test ' + test) #TODO
  if test=='018610':log('WARNING: skip test ' + test) #TODO
  if test=='018611':log('WARNING: skip test ' + test) #TODO
  if test=='018612':tests['b210']['alu']['tdd'][ '5']['3276']['tcp']['ul']=True
  if test=='018613':tests['b210']['alu']['tdd']['10']['3276']['tcp']['ul']=True
  if test=='018614':tests['b210']['alu']['tdd']['20']['3276']['tcp']['ul']=True
  if test=='018615':tests['b210']['alu']['tdd'][ '5']['3276']['tcp']['dl']=True
  if test=='018616':tests['b210']['alu']['tdd']['10']['3276']['tcp']['dl']=True
  if test=='018617':tests['b210']['alu']['tdd']['20']['3276']['tcp']['dl']=True
  if test=='018618':log('WARNING: skip test ' + test) #TODO
  if test=='018619':log('WARNING: skip test ' + test) #TODO
  if test=='018620':log('WARNING: skip test ' + test) #TODO
  if test=='018621':log('WARNING: skip test ' + test) #TODO
  if test=='018622':log('WARNING: skip test ' + test) #TODO
  if test=='018623':log('WARNING: skip test ' + test) #TODO

  if test=='025500':log('WARNING: skip test ' + test) #TODO
  if test=='025501':log('WARNING: skip test ' + test) #TODO
  if test=='025502':log('WARNING: skip test ' + test) #TODO
  if test=='025503':log('WARNING: skip test ' + test) #TODO
  if test=='025504':log('WARNING: skip test ' + test) #TODO
  if test=='025505':log('WARNING: skip test ' + test) #TODO
  if test=='025506':log('WARNING: skip test ' + test) #TODO
  if test=='025507':log('WARNING: skip test ' + test) #TODO
  if test=='025508':log('WARNING: skip test ' + test) #TODO
  if test=='025509':log('WARNING: skip test ' + test) #TODO
  if test=='025510':log('WARNING: skip test ' + test) #TODO
  if test=='025511':log('WARNING: skip test ' + test) #TODO
  if test=='025512':log('WARNING: skip test ' + test) #TODO
  if test=='025513':log('WARNING: skip test ' + test) #TODO
  if test=='025514':log('WARNING: skip test ' + test) #TODO
  if test=='025515':log('WARNING: skip test ' + test) #TODO
  if test=='025516':log('WARNING: skip test ' + test) #TODO
  if test=='025517':log('WARNING: skip test ' + test) #TODO
  if test=='025518':log('WARNING: skip test ' + test) #TODO
  if test=='025519':log('WARNING: skip test ' + test) #TODO
  if test=='025520':log('WARNING: skip test ' + test) #TODO
  if test=='025521':log('WARNING: skip test ' + test) #TODO
  if test=='025522':log('WARNING: skip test ' + test) #TODO
  if test=='025523':log('WARNING: skip test ' + test) #TODO

  if test=='025700':log('WARNING: skip test ' + test) #TODO
  if test=='025701':log('WARNING: skip test ' + test) #TODO
  if test=='025702':log('WARNING: skip test ' + test) #TODO
  if test=='025703':log('WARNING: skip test ' + test) #TODO
  if test=='025704':log('WARNING: skip test ' + test) #TODO
  if test=='025705':log('WARNING: skip test ' + test) #TODO

from alu_test import run_b210_alu

#B210 ALU tests

run_b210_alu(tests, openair_dir, oai_user, oai_password, env)

#for test in todo_tests:
#    action = test.findtext('class')
#    if action != 'lte-softmodem':
#        continue
#    if not "start_ltebox" in test.findtext('EPC_main_exec'):
#        continue
#    id = test.get('id')
#    log("INFO: Running ALU test " + id)
#    logdir = openair_dir + "/cmake_targets/autotests/log/" + id
#    quickshell("mkdir -p " + logdir)
#    epc_machine = test.findtext('EPC')
#    enb_machine = test.findtext('eNB')
#    ue_machine = test.findtext('UE')
#
#    #event object used to wait for several tasks at once
#    event = threading.Event()
#
#    #launch HSS, wait for prompt
#    log("INFO: " + id + ": run HSS")
#    task_hss = Task("actions/alu_hss.bash",
#                    "ALU HSS",
#                    epc_machine,
#                    oai_user,
#                    oai_password,
#                    env,
#                    logdir + "/alu_hss." + epc_machine, event=event)
#    task_hss.waitlog('S6AS_SIM-> ')
#
#    #then launch EPC, wait for connection on HSS side
#    log("INFO: " + id + ": run EPC")
#    task = Task("actions/alu_epc.bash",
#                "ALU EPC",
#                epc_machine,
#                oai_user,
#                oai_password,
#                env,
#                logdir + "/alu_epc." + epc_machine)
#    ret = task.wait()
#    if ret != 0:
#        log("ERROR: EPC start failure");
#        os._exit(1)
#    task_hss.waitlog('Connected\n')
#
#    #compile softmodem
#    log("INFO: " + id + ": compile softmodem")
#    envcomp = list(env)
#    envcomp.append('BUILD_ARGUMENTS="' +
#                   test.findtext('eNB_compile_prog_args') + '"')
#    #we don't care about BUILD_OUTPUT but it's required (TODO: change that)
#    envcomp.append('BUILD_OUTPUT=/')
#    task = Task("actions/compilation.bash",
#                "compile softmodem",
#                enb_machine,
#                oai_user,
#                oai_password,
#                envcomp,
#                logdir + "/compile_softmodem." + enb_machine)
#    ret = task.wait()
#    if ret != 0:
#        log("ERROR: softmodem compilation failure");
#        os._exit(1)
#
##    #copy wanted configuration file
##    quickshell("sshpass -p " + oai_password +
##               " scp config/enb.band7.tm1.usrpb210.conf " +
##                     oai_user + "@" + enb_machine + ":/tmp/enb.conf")
#
#    #run softmodem
#    log("INFO: " + id + ": run softmodem")
#    task_enb = Task("actions/run_enb.bash",
#                    "run softmodem",
#                    enb_machine,
#                    oai_user,
#                    oai_password,
#                    env,
#                    logdir + "/run_softmodem." + enb_machine, event=event)
#    task_enb.waitlog('got sync')
#
#    #start UE
#    log("INFO: " + id + ": start bandrich UE")
#    task_ue = Task("actions/start_bandrich.bash",
#                   "start bandrich UE",
#                   ue_machine,
#                   oai_user,
#                   oai_password,
#                   env,
#                   logdir + "/start_bandrich." + ue_machine, event=event)
#    task_ue.waitlog("local  IP address", event=event)
#
#    event.wait()
#
#    #at this point one task has died or we have the line in the log
#    if task_ue.waitlog_state != WAITLOG_SUCCESS:
#        log("ERROR: " + id + ": bandrich UE did not connect")
#        os._exit(1)
#
#    event.clear()
#
#    if (    not task_enb.alive() or
#            not task_hss.alive() or
#            not task_ue.alive()):
#        log("ERROR: " + id + ": eNB or UE tasks died")
#        os._exit(1)
#
#    #get bandrich UE IP
#    l = open(task_ue.logfile, "r").read()
#    ue_ip = re.search("local  IP address (.*)\n", l).groups()[0]
#    log("INFO: " + id + ": bandrich UE IP address: " + ue_ip)
#
#    #run traffic
#    log("INFO: " + id + ": run downlink TCP traffic")
#
#    log("INFO: " + id + ":     launch server")
#    task_traffic_ue = Task("actions/downlink_bandrich.bash",
#                           "start iperf on bandrich UE as server",
#                           ue_machine,
#                           oai_user,
#                           oai_password,
#                           env,
#                           logdir + "/downlink_bandrich." + ue_machine,
#                           event=event)
#    task_traffic_ue.waitlog("Server listening on TCP port 5001")
#
#    log("INFO: " + id + ":     launch client")
#    envepc = list(env)
#    envepc.append("UE_IP=" + ue_ip)
#    task = Task("actions/downlink_epc.bash",
#                "start iperf on EPC as client",
#                epc_machine,
#                oai_user,
#                oai_password,
#                envepc,
#                logdir + "/downlink_epc." + epc_machine, event=event)
#    log("INFO: " + id + ":     wait for client (or some error)")
#
#    event.wait()
#    log("DEBUG: event.wait() done")
#
#    if (    not task_enb.alive() or
#            not task_hss.alive() or
#            not task_ue.alive()):
#        log("ERROR: unexpected task exited, test failed, kill all")
#        if task.alive():
#            task.kill()
#        if task_enb.alive():
#            task_enb.kill()
#        if task_ue.alive():
#            task_ue.kill()
#
#    ret = task.wait()
#    if ret != 0:
#        log("ERROR: " + id + ": downlink traffic failed")
#        #not sure if we have to quit here or not
#        #os._exit(1)
#
#    #stop downlink server
#    log("INFO: " + id + ":     stop server (kill ssh connection)")
#    task_traffic_ue.kill()
#    log("INFO: " + id + ":     wait for server to quit")
#    task_traffic_ue.wait()
#
#    #stop UE
#    log("INFO: " + id + ": stop bandrich UE")
#    task_ue.sendnow("%c" % 3)
#    ret = task_ue.wait()
#    if ret != 0:
#        log("ERROR: " + id + ": task bandrich UE failed")
#        #not sure if we have to quit here or not
#        #os._exit(1)
#
#    #stop softmodem
#    log("INFO: " + id + ": stop softmodem")
#    task_enb.sendnow("%c" % 3)
#    ret = task_enb.wait()
#    if ret != 0:
#        log("ERROR: " + id + ": softmodem failed")
#        #not sure if we have to quit here or not
#        #os._exit(1)
#
#    #stop EPC, wait for disconnection on HSS side
#    log("INFO: " + id + ": stop EPC")
#    task = Task("actions/alu_epc_stop.bash",
#                "ALU EPC stop",
#                epc_machine,
#                oai_user,
#                oai_password,
#                env,
#                logdir + "/alu_epc_stop." + epc_machine)
#    ret = task.wait()
#    if ret != 0:
#        log("ERROR: " + id + ": ALU EPC stop failed")
#        os._exit(1)
#    task_hss.waitlog('Disconnected\n')
#
#    log("INFO: " + id + ": stop HSS")
#    task_hss.sendnow("exit\n")
#    ret = task_hss.wait()
#    if ret != 0:
#        log("ERROR: " + id + ": ALU HSS failed")
#        os._exit(1)

import utils
log(utils.GREEN + "GOODBYE" + utils.RESET)
#os._exit(0)

#run lte softmodem tests
