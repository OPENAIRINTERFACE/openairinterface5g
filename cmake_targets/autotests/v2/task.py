import os, time, threading, sys, os, errno, select
from connection import connection

import utils
from utils import log

#this class is to communicate between reader and sender threads
#the reader increments a count each time it receives the prompt
#and wakes up the sender
#it also indicates when it exits so the sender can exit too
#the sender (calling get) waits for the count to be greated than passed
#argument or if reader exited
#it returns argument+1 if things okay, -1 if reader exited
class Counter:
    def __init__(self):
        self.count = 0
        self.cond = threading.Condition()

    def inc(self):
        self.cond.acquire()
        self.count = self.count + 1
        self.cond.notify()
        self.cond.release()

    def set(self, value):
        self.cond.acquire()
        self.count = value
        self.cond.notify()
        self.cond.release()

    def get(self, current):
        self.cond.acquire()
        while True:
            if self.count == -1:
                ret = -1
                break
            if self.count >= current + 1:
                ret = current + 1
                break
            self.cond.wait()
        self.cond.release()
        return ret

#this class is used for the main application to wait for some specific
#output from the remote program (typically the ALU HSS prompt, indicating
#it is ready and we can proceed further)
#(too much classes...)
class ProducerConsumer:
    def __init__(self):
        self.count = 0
        self.cond = threading.Condition()

    def add(self, v):
        self.cond.acquire()
        self.count = self.count + v
        self.cond.notify()
        self.cond.release()

    def set(self, v):
        self.cond.acquire()
        self.count = v
        self.cond.notify()
        self.cond.release()

    def get(self, old):
        self.cond.acquire()
        while True:
            if self.count == -1:
                ret = -1
                break
            if self.count > old:
                ret = self.count
                break
            self.cond.wait()
        self.cond.release()
        return ret

#this thread gets input from the child process of the task
#it removes the prompts it gets (be carefully to use a prompt
#that will not be output of any command otherwise you dead)
class ReaderThread(threading.Thread):
    def __init__(self, fdin, logfile, prompt, prompt_counter, producer):
        threading.Thread.__init__(self)
        self.fdin = fdin
        self.logfile = logfile
        self.prompt_counter = prompt_counter
        self.prompt = prompt
        self.promptsize = len(prompt)
        self.stack = ""
        self.stacksize = 0
        self.producer = producer

    def run(self):
        try:
            outfile = open(self.logfile, "w")
        except BaseException, e:
            log("ERROR: ReaderThread: " + self.logfile + ": " + str(e))
            os._exit(1)
        while True:
            try:
               (a, b, c) = select.select([ self.fdin ], [], [ self.fdin ])
            except BaseException, e:
               log("ERROR: ReaderThread: " + self.logfile +
                   ": select failed: " + str(e))
               os._exit(1)
            try:
                z = os.read(self.fdin, 1024)
            except OSError, e:
                if e.errno == errno.EIO:
                  #pipe has died, quit the thread
                  break
                else:
                  log("ERROR: ReaderThread: " + self.logfile +
                      ": unhandled error: " + str(e))
            except BaseException, e:
                log("ERROR: ReaderThread: " + self.logfile +
                    ": unhandled error: " + str(e))
                break
            try:
                produced = 0
                #this part is to remove the prompt
                for x in z:
                    if x == self.prompt[self.stacksize]:
                        self.stack = self.stack + x
                        self.stacksize = self.stacksize + 1
                        if self.stacksize == self.promptsize:
                            self.prompt_counter.inc()
                            self.stack = ""
                            self.stacksize = 0
                    else:
                        outfile.write(self.stack)
                        outfile.write(x)
                        produced = produced + len(self.stack) + len(x)
                        self.stack = ""
                        self.stacksize = 0
                outfile.flush()
                self.producer.add(produced)
            except BaseException, e:
                log("ERROR: ReaderThread: " + self.logfile + ": " + str(e))
                os._exit(1)
        try:
            outfile.close()
        except BaseException, e:
            log("ERROR: ReaderThread: " + self.logfile + ": " + str(e))
            os._exit(1)
        #close the pipe, don't care about errors
        try:
            os.close(self.fdin)
        except:
            pass
        #signal sender to quit
        self.prompt_counter.set(-1)
        self.producer.set(-1)

class SenderQuit(Exception):
    pass

#this thread sends commands to the child process of the task
#it waits for the prompt between each command
#'event' is used for the main thread to wait for one of several tasks
#to quit, meaning error or end-of-test
class SenderThread(threading.Thread):
    def __init__(self, fdout, prompt_counter, connection, env, action,
                 description, prompt, event=None):
        threading.Thread.__init__(self)
        self.fdin = fdout
        self.prompt_counter = prompt_counter
        self.connection = connection
        self.env = env
        self.action = action
        self.description = description
        self.prompt = prompt
        self.count = 0
        self.event = event
        self.alive = True

    def wait_prompt(self):
        self.count = self.prompt_counter.get(self.count)
        if self.count == -1:
            raise SenderQuit()

    def _run(self):
        self.connection.send('export PS1=' + self.prompt + '\n')
        self.wait_prompt()
        self.connection.send('set +o emacs\n')
        self.wait_prompt()
        self.connection.send('echo\n')
        self.wait_prompt()
        self.connection.send('echo\n')
        self.wait_prompt()
        self.connection.send("echo -e '" + utils.GREEN +
                             '---------------------------------------------'
                             + utils.RESET + "'\n")
        self.wait_prompt()
        self.connection.send('echo\n')
        self.wait_prompt()
        self.connection.send("echo -n -e '" + utils.YELLOW +
                             "COMMANDS START: " +
                             utils.RESET + "'\n")
        self.wait_prompt()
        self.connection.send('date\n')
        self.wait_prompt()

        for l in self.env:
            self.connection.send('export ' + l + '\n')
            self.wait_prompt()

        with open(self.action) as f:
            for line in f:
                self.connection.send("echo -n -e '" + utils.GREEN +
                                     "RUNNING: " + utils.RESET + "'\n")
                self.wait_prompt()
                self.connection.send("echo '" +
                                     line.replace('\n','')
                                         .replace("'", "'\\''") + "'\n")
                self.wait_prompt()
                self.connection.send(line)
                self.wait_prompt()
                self.connection.send("if [ $? != 0 ]; then " +
                                     "echo -e '" + utils.RED +
                                     "TEST_SETUP_ERROR: " + utils.RESET +
                                     "last command failed, exiting'; " +
                                     "date; exit 1; fi\n")
                self.wait_prompt()

        self.connection.send("echo -n -e '" + utils.YELLOW +
                             "COMMANDS DONE: " +
                             utils.RESET + "'\n")
        self.wait_prompt()
        self.connection.send('date\n')
        self.wait_prompt()
        self.connection.send("echo -e '" + utils.GREEN +
                             "TEST_SETUP_SUCCESS" + utils.RESET + "'\n")
        self.wait_prompt()
        self.connection.send('exit\n')

    def run(self):
        try:
            self._run()
        except SenderQuit:
            log("WARNING: '" + self.description + "' exits too early?")
            pass
        except BaseException, e:
            log("ERROR: task failed:    " + str(e))
            log("ERROR: action is:      " + self.action)
            log("ERROR: description is: " + self.description)
            os._exit(1)

        self.alive = False

        if self.event != None:
            self.event.set()

WAITLOG_RUNNING = 0
WAITLOG_SUCCESS = 1
WAITLOG_FAILURE = 2

class WaitlogFailed(Exception):
    pass

#'event' is used by main thread to wait for any of several threads to quit.
#'Task' passes it the 'SenderThread' above, which sets it when it finishes.
class Task:
    def __init__(self, action, description, machine, user, password, env,
                 logfile, post_action = None, event=None):
        self.action      = action
        self.description = description
        self.machine     = machine
        self.user        = user
        self.password    = password
        self.post_action = post_action
        self.producer    = ProducerConsumer()
        self.logfile     = logfile

        prompt = "__OAI_TEST_SETUP_PROMPT__:"
        prompt_counter = Counter()

        self.connection = connection(description, machine, user, password)

        self.reader = ReaderThread(self.connection.fd, logfile, prompt,
                                   prompt_counter, self.producer)
        self.reader.start()

        self.sender = SenderThread(self.connection.fd, prompt_counter,
                                   self.connection, env, action, description,
                                   prompt, event)
        self.sender.start()

    def wait(self, timeout=-1):
        if self.connection.active == False:
            return self.connection.retcode
        try:
            (pid, ret) = os.waitpid(self.connection.pid, 0)
        except KeyboardInterrupt, e:
            log("ERROR: ctrl+c catched! " + str(e))
            os._exit(1)
        except BaseException, e:
            log("ERROR: " + str(e))
            os._exit(1)
        self.sender.join()
        self.reader.join()
        return ret

    #this function should not be called, it is used internally by 'waitlog'
    #in mode 'background thread'
    #TODO: join() the thread at some point
    def _waitlog_thread(self, task, s):
        consumed = 0
        while True:
            consumed = task.producer.get(consumed)
            if consumed == -1:
                log("ERROR: string '" + s + "' not found in logfile " +
                    task.logfile)
                task.waitlog_state = WAITLOG_FAILURE
                task.waitlog_event.set()
                return
            if s in open(task.logfile).read():
                task.waitlog_state = WAITLOG_SUCCESS
                task.waitlog_event.set()
                return

    #two ways to wait for a string in the log file:
    #  - blocking way
    #  - background thread, using an Event to signal success/failure
    def waitlog(self, s, event=None):
        if event != None:
            self.waitlog_state = WAITLOG_RUNNING
            self.waitlog_event = event
            self.waitlog_thread = \
                threading.Thread(target=self._waitlog_thread,
                                 args=(self, s))
            self.waitlog_thread.start()
            return

        #TODO: optimize, do not process all the file at each wakeup
        consumed = 0
        while True:
            consumed = self.producer.get(consumed)
            if consumed == -1:
                log("ERROR: string '" + s + "' not found in logfile " +
                    self.logfile)
                raise WaitlogFailed()
            if s in open(self.logfile).read():
                return

    def sendnow(self, x):
        self.connection.send(x)

    def alive(self):
        return self.sender.alive

    def kill(self):
        self.connection.kill()
        #put some error log in logfile, for verbosity
        try:
            f = open(self.logfile, "a+")
            f.write("\n\n" + utils.RED + "TEST_SETUP_ERROR: " + utils.RESET +
                    "task killed by test setup\n\n");
            close(f)
        except BaseException, e:
            pass

    def postaction(self):
        if self.post_action != None:
            out = utils.quickshell(self.post_action)
            if len(out):
                log("INFO: task '" + self.description +
                    "' post_action '" + self.post_action + "' says: ")
                for l in out.splitlines():
                    log("INFO:     " + l)
