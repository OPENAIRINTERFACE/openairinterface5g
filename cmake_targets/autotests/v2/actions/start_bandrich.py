import time

from modem import quit, Modem

try:
    modem = Modem("/dev/bandrich.control")

    #test that modem is there
    print "INFO: check modem's presence"
    modem.send('AT')
    if modem.wait().ret != True:
        print "ERROR: no modem?"
        quit(1)

    #activate the modem, be brutal and reset it too!
    print "INFO: reset and activate the modem"
    modem.send('AT+CFUN=1,1')
    if modem.wait().ret != True:
        print "ERROR: failed asking modem for activation"
        quit(1)

    #modem has gone! wait for it to pop up again
    modem.modem_reset_cycle()

    #wait for modem to be connected
    #timeout after one minute
    print "INFO: wait for modem to be connected (timeout: one minute)"
    start_time = time.time()
    while True:
        modem.send('AT+CGATT?')
        r = modem.wait()
        if r.ret != True:
            print "ERROR: failed checking attachment status of modem"
            quit(1)
        if "+CGATT: 1" in r.data:
            break
        if not "CGATT: 0" in r.data:
            print "ERROR: bad data when checking attachment status of modem"
            quit(1)
        time.sleep(0.1)
        if time.time() > start_time + 60:
            print "ERROR: modem not connected after one minute, close modem"
            modem.send('AT+CFUN=4')
            r = modem.wait()
            if r.ret != True:
                print "ERROR: closing modem failed"
            quit(1)

    print "INFO: modem is connected"

except BaseException, e:
    print "ERROR: " + str(e)
    quit(1)

quit(0)
