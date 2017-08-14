import subprocess, os, thread, sys, time

#logging facitiliy
#TODO: simplify (a first version redefined stdout, then we replaced 'print'
#instead, keeping stuff as is)
class Logger:
   def __init__(self, stream):
       self.stream = stream
       self.start_of_line = True
       self.lock = thread.allocate_lock()
       openair_dir = os.environ.get('OPENAIR_DIR')
       if openair_dir == None:
           print "FATAL: no OPENAIR_DIR"
           os._exit(1)
       try:
           self.logfile = open(openair_dir +
                            "/cmake_targets/autotests/log/python.stdout", "w")
       except BaseException, e:
           print "FATAL:  cannot create log file"
           print e
           os._exit(1)
   def put(self, data):
       self.stream.write(data)
       self.logfile.write(data)
   def write(self, data):
       self.lock.acquire()
       for c in data:
           if self.start_of_line:
               self.start_of_line = False
               t = time.strftime("%H:%M:%S", time.localtime()) + ": "
               self.stream.write(t)
               self.logfile.write(t)
           self.put(c)
           if c == '\n':
               self.start_of_line = True
       self.stream.flush()
       self.logfile.flush()
       self.lock.release()
logger = Logger(sys.stdout)
def log(x):
    logger.write(x + "\n")

#check if given test is in list
#it is in list if one of the strings in 'list' is at the beginning of 'test'
def test_in_list(test, list):
    for check in list:
        check=check.replace('+','')
        if (test.startswith(check)):
            return True
    return False

#run a local command in a shell
def quickshell(command):
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    (retout, reterr) = process.communicate()
    if (process.returncode != 0):
        log("ERROR: shell command failed: " + command)
        if len(retout):
            log("ERROR: command says: ")
            for l in retout.splitlines():
                log("ERROR:     " + l)
        os._exit(1)
    return retout

RED="\x1b[31m"
GREEN="\x1b[32m"
YELLOW="\x1b[33m"
RESET="\x1b[m"

#an exception raised when a test fails
class TestFailed(Exception):
    pass

#this function returns True if a test in 'x' is set to True
#to be used as: if do_tests(tests['b210']['alu']) ...
def do_tests(x):
    if type(x) == list:
        for v in x:
            if do_tests(v): return True
        return False
    if type(x) == dict: return do_tests(x.values())
    if x == True: return True
    return False
