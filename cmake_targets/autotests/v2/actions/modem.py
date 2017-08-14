import sys, os, select, re, time

def quit(r):
    sys.stdout.flush()
    os._exit(r)

class ModemResponse:
    def __init__(self, retcode, retstring):
        self.ret = retcode
        self.data = retstring

class Modem:
    def open(self):
        self.i = os.open(self.devname, os.O_RDONLY)
        self.o = os.open(self.devname, os.O_WRONLY)
        #clear output of modem, if any is pending (not sure of this)
        while True:
            (ri, ro, re) = select.select([self.i], [], [self.i], 0)
            if len(ri) == 0:
                break
            l = os.read(self.i, 65536)
            print "WARNING: modem had unread data: '" + \
                  l.replace('\r', '\\r') + "'"

    def __init__(self, devname):
        self.devname = devname
        self.i = -1
        self.o = -1
        self.open()

    def send(self, s):
        print "DEBUG: SEND TO MODEM: '" + s + "'"
        os.write(self.o, s+"\r")

    def recv(self):
        return os.read(self.i, 65536)

    def wait(self):
        ok         = '\r\nOK\r\n'
        error      = '\r\nERROR\r\n'
        cme_error  = '\r\nCME ERROR:[^\r]*\r\n'
        no_carrier = '\r\nNO CARRIER\r\n'
        l = ''
        while True:
            l = l + self.recv()
            print "DEBUG: CURRENT MODEM RESPONSE: '" + \
                  l.replace('\r','\\r').replace('\n','\\n') + "'"

            #AT returned 'things' are "\r\nXXXX\r\n", look for that.
            #Check if last one matches 'ok', 'error' or 'cme_error'.
            #(Hopefully this is enough and no other reply is possible.)
            #This code accepts invalid responses from modem, ie. all
            #that does not fit in the 'findall' is thrashed away, maybe
            #we want to do something in this case?

            res = re.findall('\r\n[^\r]*\r\n', l)
            if len(res) == 0:
                print "DEBUG: NO MATCH: wait for more input from modem"
                continue
            last_res = res[len(res)-1]
            print "DEBUG: CURRENT LAST LINE: '" + \
                  last_res.replace('\r','\\r').replace('\n','\\n')+"'"
            if re.match(ok, last_res) != None:
                return ModemResponse(True, l)
            if (   re.match(error,      last_res) != None or
                   re.match(cme_error,  last_res) != None or
                   re.match(no_carrier, last_res) != None):
                return ModemResponse(False, l)
        #TODO purge?
        #re.purge()

    def modem_reset_cycle(self):
        #close all
        os.close(self.i)
        os.close(self.o)
        self.i = -1
        self.o = -1

        print "DEBUG: RESET CYCLE: wait for modem to go away"
        #wait for file descriptor to go away
        while True:
            try:
                test = os.open(self.devname, os.O_RDONLY)
                os.close(test)
                time.sleep(0.1)
            except BaseException, e:
                break
        print "DEBUG: RESET CYCLE: modem has gone away"

        print "DEBUG: RESET CYCLE: wait for modem to come back"
        #wait for file descriptor to be back, try to open it over and over
        #TODO: use inotify here? (it's not in basic python as it seems)
        while True:
            try:
                test = os.open(self.devname, os.O_RDONLY)
                os.close(test)
                break
            except BaseException, e:
                time.sleep(0.1)
        print "DEBUG: RESET CYCLE: modem is back"

        #back to business
        self.open()
