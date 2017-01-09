import time

from modem import quit, Modem

try:
    modem = Modem("/dev/bandrich.control")

    #test that modem is there
    print "INFO: check modem's presence"
    modem.send('AT')
    r = modem.wait()
    if r.ret != True and "NO CARRIER" not in r.data:
        print "ERROR: no modem?"
        quit(1)
    if "NO CARRIER" in r.data:
        print "WARNING: 'NO CARRIER' detected, not sure if handled correctly"

    #deactivate the modem
    print "INFO: deactivate the modem"
    modem.send('AT+CFUN=4')
    if modem.wait().ret != True:
        print "ERROR: failed asking modem for deactivation"
        quit(1)

except BaseException, e:
    print "ERROR: " + str(e)
    quit(1)

quit(0)
