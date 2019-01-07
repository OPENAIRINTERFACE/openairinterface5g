import os, subprocess, time, fcntl, termios, tty, signal, thread

from utils import log

class connection:
    def __init__(self, description, host, user, password):
        self.description = description
        self.host = host
        self.user = user
        self.password = password
        self.sendlock = thread.allocate_lock()

        try:
            (pid, fd) = os.forkpty()
        except BaseException, e:
            log("ERROR: forkpty for '" + description + "': " + str(e))
            (pid, fd) = (-1, -1)

        if pid == -1:
            log("ERROR: creating connection for '" + description + "'")
            os._exit(1)

        # child process, run ssh
        if pid == 0:
            try:
                os.execvp('sshpass', ['sshpass', '-p', password,
                          'ssh', user + '@' + host])
            except BaseException, e:
                log("ERROR: execvp for '" + description + "': " + str(e))
            log("ERROR: execvp failed for '" + description + "'")
            os._exit(1)

        # parent process
        # make the TTY raw to avoid getting input printed and no ^M
        try:
            tty.setraw(fd, termios.TCSANOW)
        except BaseException, e:
            log("ERROR: failed configuring TTY: " + str(e))
            os._exit(1)

#        try:
#            fcntl.fcntl(fd, fcntl.F_SETFL,
#                        fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
#        except:
#            log("ERROR: fcntl failed for '" + description + "'")
#            os._exit(1)

        self.pid = pid
        self.fd = fd
        self.active = True
        self.retcode = -1

    def send(self, string):
        if self.active == False:
            log("ERROR: send: child is dead for '" + self.description + "'")
            return -1

        try:
            (pid, out) = os.waitpid(self.pid, os.WNOHANG)
        except BaseException, e:
            log("ERROR: send: waitpid failed for '" + self.description +
                "': " + str(e))
            (pid, out) = (self.pid, 1)
        if pid != 0:
            log("ERROR: send: child process dead for '" +
                self.description + "'")
            try:
                os.close(self.fd)
            except BaseException, e:
                #we don't care about errors at this point
                pass
            self.active = False
            self.retcode = out
            return -1

        self.sendlock.acquire()

        length = len(string)
        while length != 0:
            try:
                ret = os.write(self.fd, string)
            except BaseException, e:
                log("ERROR: send fails for '" + self.description + "': " +
                    str(e))
                os._exit(1)

            if ret == 0:
                log("ERROR: send: write returns 0 for '" +
                    self.description + "'")
                os._exit(1)

            length = length - ret
            string = string[ret:]

        self.sendlock.release()

        return 0

    def kill(self, signal=signal.SIGTERM):
        log("INFO: kill connection '" + self.description + "'")
        try:
            os.kill(self.pid, signal)
        except BaseException, e:
            log("ERROR: connection.kill: " + str(e))
