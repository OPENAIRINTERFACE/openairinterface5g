import threading, os, re

from utils import quickshell, log, TestFailed, do_tests
from task import Task, WAITLOG_SUCCESS

class alu_test:
    def __init__(self,
                 epc, enb, ue,
                 openair,
                 user, password,
                 log_subdir,
                 env,
                 enb_rru = "",
                 enb_rcc = ""):
        self.epc_machine      = epc
        self.enb_machine      = enb
        self.ue_machine       = ue
        self.openair_dir      = openair
        self.oai_user         = user
        self.oai_password     = password
        self.env              = env
        self.enb_rru_machine  = enb_rru
        self.enb_rcc_machine  = enb_rcc

        self.task_hss     = None
        self.task_enb     = None
        self.task_ue      = None
        self.task_rru_enb = None
        self.task_rcc_enb = None

        self.logdir = openair + '/cmake_targets/autotests/log/' + log_subdir
        quickshell('mkdir -p ' + self.logdir)

        #event object used to wait for several tasks at once
        self.event = threading.Event()

    ##########################################################################
    # finish
    ##########################################################################
    def finish(self):
        #brutally kill tasks still running
        #TODO: call 'wait', some processes may still be there as zombies
        if self.task_hss != None and self.task_hss.alive():
            self.task_hss.kill()
        if self.task_enb != None and self.task_enb.alive():
            self.task_enb.kill()
        if self.task_ue != None and self.task_ue.alive():
            self.task_ue.kill()
        if self.task_rru_enb != None and self.task_rru_enb.alive():
            self.task_rru_enb.kill()
        if self.task_rcc_enb != None and self.task_rcc_enb.alive():
            self.task_rcc_enb.kill()

    ##########################################################################
    # start_epc
    ##########################################################################
    def start_epc(self):
        #launch HSS, wait for prompt
        log("INFO: ALU test: run HSS")
        self.task_hss = Task("actions/alu_hss.bash",
                "alu_hss",
                self.epc_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/alu_hss." + self.epc_machine,
                event=self.event)
        self.task_hss.waitlog('S6AS_SIM-> ')

        #then launch EPC, wait for connection on HSS side
        log("INFO: ALU test: run EPC")
        task_epc = Task("actions/alu_epc.bash",
                "ALU EPC",
                self.epc_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/alu_epc." + self.epc_machine)
        ret = task_epc.wait()
        if ret != 0:
            log("ERROR: EPC start failure");
            raise TestFailed()
        self.task_hss.waitlog('Connected\n')

    ##########################################################################
    # stop_epc
    ##########################################################################
    def stop_epc(self):
        #stop EPC, wait for disconnection on HSS side
        log("INFO: ALU test: stop EPC")
        task_epc = Task("actions/alu_epc_stop.bash",
                "alu_epc_stop",
                self.epc_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/alu_epc_stop." + self.epc_machine)
        ret = task_epc.wait()
        if ret != 0:
            log("ERROR: ALU test: ALU EPC stop failed")
            raise TestFailed()
        self.task_hss.waitlog('Disconnected\n')

        log("INFO: ALU test: stop HSS")
        self.task_hss.sendnow("exit\n")
        ret = self.task_hss.wait()
        if ret != 0:
            log("ERROR: ALU test: ALU HSS failed")
            raise TestFailed()

    ##########################################################################
    # compile_enb
    ##########################################################################
    def compile_enb(self, build_arguments, log_suffix=""):
        log("INFO: ALU test: compile softmodem on " + self.enb_machine)
        envcomp = list(self.env)
        envcomp.append('BUILD_ARGUMENTS="' + build_arguments + '"')
        #we don't care about BUILD_OUTPUT but required (TODO: change that)
        envcomp.append('BUILD_OUTPUT=/')
        logdir = self.logdir + "/compile_log"
        remote_files = "'/tmp/oai_test_setup/oai/cmake_targets/log/*'"
        post_action = "mkdir -p "+ logdir + \
                " && sshpass -p " + self.oai_password + \
                " scp -r " + self.oai_user + \
                "@" + self.enb_machine + ":" + remote_files + " " + logdir + \
                " || true"
        task = Task("actions/compilation.bash",
                "compile_softmodem",
                self.enb_machine,
                self.oai_user,
                self.oai_password,
                envcomp,
                self.logdir + "/compile_softmodem." + log_suffix + \
                    self.enb_machine,
                post_action=post_action)
        ret = task.wait()
        if ret != 0:
            log("ERROR: softmodem compilation failure");
            raise TestFailed()
        task.postaction()

    ##########################################################################
    # start_enb
    ##########################################################################
    def start_enb(self, config_file):
        #copy wanted configuration file
        quickshell("sshpass -p " + self.oai_password +
                   " scp config/" + config_file + " " +
                   self.oai_user + "@" + self.enb_machine + ":/tmp/enb.conf")

        #run softmodem
        log("INFO: ALU test: run softmodem with configuration file " +
            config_file)
        self.task_enb = Task("actions/run_enb.bash",
                "run_softmodem",
                self.enb_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/run_softmodem." + self.enb_machine,
                event=self.event)
        self.task_enb.waitlog('got sync')

    ##########################################################################
    # stop_enb
    ##########################################################################
    def stop_enb(self):
        log("INFO: ALU test: stop softmodem")
        self.task_enb.sendnow("%c" % 3)
        ret = self.task_enb.wait()
        if ret != 0:
            log("ERROR: ALU test: softmodem failed")
            #not sure if we have to quit here or not
            #os._exit(1)

    ##########################################################################
    # start_enb_rru_rcc
    ##########################################################################
    def start_enb_rru_rcc(self, rru_config_file, rcc_config_file):
        #copy wanted configuration files
        quickshell("sshpass -p " + self.oai_password +
                   " scp config/" + rru_config_file + " " +
                   self.oai_user + "@" + self.enb_rru_machine+":/tmp/enb.conf")
        quickshell("sshpass -p " + self.oai_password +
                   " scp config/" + rcc_config_file + " " +
                   self.oai_user + "@" + self.enb_rcc_machine+":/tmp/enb.conf")

        #run RRU/RCC softmodem
        log("INFO: ALU test: run RRU softmodem with configuration file " +
            rru_config_file)
        self.task_rru_enb = Task("actions/run_enb.bash",
                "run_RRU_softmodem",
                self.enb_rru_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/run_softmodem." + self.enb_rru_machine,
                event=self.event)
        self.task_rru_enb.waitlog('[RRH] binding to')
        log("INFO: ALU test: run RCC softmodem with configuration file " +
            rcc_config_file)
        self.task_rcc_enb = Task("actions/run_enb.bash",
                "run_RCC_softmodem",
                self.enb_rcc_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/run_softmodem." + self.enb_rcc_machine,
                event=self.event)
        self.task_rcc_enb.waitlog('[BBU] local ip addr')
        #wait for RRU and RCC to be connected
        self.task_rru_enb.waitlog('devices ok (eNB_thread_asynch_rx)')

    ##########################################################################
    # stop_enb_rru_rcc
    ##########################################################################
    def stop_enb_rru_rcc(self):
        log("INFO: ALU test: stop RRU/RCC softmodem")
        self.task_rru_enb.sendnow("%c" % 3)
        ret = self.task_rru_enb.wait()
        if ret != 0:
            log("ERROR: ALU test: RRU softmodem failed")
            #not sure if we have to quit here or not
            #os._exit(1)
        self.task_rcc_enb.sendnow("%c" % 3)
        ret = self.task_rcc_enb.wait()
        if ret != 0:
            log("ERROR: ALU test: RCC softmodem failed")
            #not sure if we have to quit here or not
            #os._exit(1)

    ##########################################################################
    # start_ue
    ##########################################################################
    def start_ue(self, ue):
        log("INFO: ALU test: start " + ue + " UE")
        self.task_ue = Task("actions/start_" + ue + ".bash",
                "start_" + ue,
                self.ue_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/start_" + ue + "." + self.ue_machine,
                event=self.event)
        self.task_ue.waitlog("local  IP address", event=self.event)
        self.event.wait()

        #at this point one task has died or we have the line in the log
        if self.task_ue.waitlog_state != WAITLOG_SUCCESS:
            log("ERROR: ALU test: " + ue + " UE did not connect")
            raise TestFailed()

        self.event.clear()

        if (    (self.task_enb != None and not self.task_enb.alive()) or
                (self.task_rru_enb!=None and not self.task_rru_enb.alive()) or
                (self.task_rcc_enb!=None and not self.task_rcc_enb.alive()) or
                not self.task_hss.alive() or
                not self.task_ue.alive()):
            log("ERROR: ALU test: eNB, HSS or UE task died")
            raise TestFailed()

        #get UE IP
        l = open(self.task_ue.logfile, "r").read()
        self.ue_ip = re.search("local  IP address (.*)\n", l) \
                       .groups()[0]
        log("INFO: ALU test: " + ue + " UE IP address: " + self.ue_ip)

    ##########################################################################
    # stop_ue
    ##########################################################################
    def stop_ue(self, ue):
        log("INFO: ALU test: stop " + ue + " UE")
        self.task_ue.sendnow("%c" % 3)
        ret = self.task_ue.wait()
        if ret != 0:
            log("ERROR: ALU test: task " + ue + " UE failed")
            #not sure if we have to quit here or not
            #os._exit(1)

    ##########################################################################
    # _do_traffic (internal function, do not call out of the class)
    ##########################################################################
    def _do_traffic(self, name,
            server_code, server_machine, server_ip,
            client_code, client_machine,
            waitlog,
            server_logfile, client_logfile,
            udp_bandwidth=None):
        log("INFO: ALU test: run traffic: " + name)

        log("INFO: ALU test:     launch server")
        task_traffic_server = Task("actions/" + server_code + ".bash",
                server_logfile,
                server_machine,
                self.oai_user,
                self.oai_password,
                self.env,
                self.logdir + "/" + server_logfile + "." + server_machine,
                event=self.event)
        task_traffic_server.waitlog(waitlog)

        env = list(self.env)
        if udp_bandwidth != None:
            env.append("UDP_BANDWIDTH=" + udp_bandwidth)
        env.append("SERVER_IP=" + server_ip)

        log("INFO: ALU test:     launch client")
        task_traffic_client = Task("actions/" + client_code + ".bash",
                client_logfile,
                client_machine,
                self.oai_user,
                self.oai_password,
                env,
                self.logdir + "/" + client_logfile + "." + client_machine,
                event=self.event)
        log("INFO: ALU test:     wait for client to finish (or some error)")

        self.event.wait()
        log("DEBUG: event.wait() done")

        if (    (self.task_enb != None and not self.task_enb.alive()) or
                (self.task_rru_enb!=None and not self.task_rru_enb.alive()) or
                (self.task_rcc_enb!=None and not self.task_rcc_enb.alive()) or
                not self.task_hss.alive() or
                not self.task_ue.alive()):
            log("ERROR: unexpected task exited, test failed, kill all")
            if task_traffic_client.alive():
                task_traffic_client.kill()
            if (self.task_enb != None and self.task_enb.alive()):
                self.task_enb.kill()
            if (self.task_rru_enb != None and self.task_rru_enb.alive()):
                self.task_rru_enb.kill()
            if (self.task_rcc_enb != None and self.task_rcc_enb.alive()):
                self.task_rcc_enb.kill()
            if self.task_ue.alive():
                self.task_ue.kill()

        ret = task_traffic_client.wait()
        if ret != 0:
            log("ERROR: ALU test: downlink traffic failed")
            #not sure if we have to quit here or not
            #os._exit(1)

        #stop downlink server
        #log("INFO: ALU test:     stop server (kill ssh connection)")
        #task_traffic_server.kill()
        log("INFO: ALU test:     stop server (ctrl+z then kill -9 %1)")
        task_traffic_server.sendnow("%ckill -9 %%1 || true\n" % 26)
        log("INFO: ALU test:     wait for server to quit")
        task_traffic_server.wait()

        self.event.clear()

        if (    (self.task_enb != None and not self.task_enb.alive()) or
                (self.task_rru_enb!=None and not self.task_rru_enb.alive()) or
                (self.task_rcc_enb!=None and not self.task_rcc_enb.alive()) or
                not self.task_hss.alive() or
                not self.task_ue.alive()):
            log("ERROR: ALU test: eNB, HSS or UE task died")
            raise TestFailed()

    ##########################################################################
    # dl_tcp
    ##########################################################################
    def dl_tcp(self, ue):
        self._do_traffic(ue + " downlink TCP",
                         "server_tcp", self.ue_machine, self.ue_ip,
                         "client_tcp", self.epc_machine,
                         "Server listening on TCP port 5001",
                         ue + "_downlink_tcp_server",
                         ue + "_downlink_tcp_client")

    ##########################################################################
    # ul_tcp
    ##########################################################################
    def ul_tcp(self, ue):
        self._do_traffic(ue + " uplink TCP",
                         "server_tcp", self.epc_machine, "192.172.0.1",
                         "client_tcp", self.ue_machine,
                         "Server listening on TCP port 5001",
                         ue + "_uplink_tcp_server",
                         ue + "_uplink_tcp_client")

    ##########################################################################
    # dl_udp
    ##########################################################################
    def dl_udp(self, ue, bandwidth):
        self._do_traffic(ue + " downlink UDP",
                         "server_udp", self.ue_machine, self.ue_ip,
                         "client_udp", self.epc_machine,
                         "Server listening on UDP port 5001",
                         ue + "_downlink_udp_server",
                         ue + "_downlink_udp_client",
                         udp_bandwidth=bandwidth)

    ##########################################################################
    # ul_udp
    ##########################################################################
    def ul_udp(self, ue, bandwidth):
        self._do_traffic(ue + " uplink UDP",
                         "server_udp", self.epc_machine, "192.172.0.1",
                         "client_udp", self.ue_machine,
                         "Server listening on UDP port 5001",
                         ue + "_uplink_udp_server",
                         ue + "_uplink_udp_client",
                         udp_bandwidth=bandwidth)

##############################################################################
# run_b210_alu
##############################################################################

def run_b210_alu(tests, openair_dir, oai_user, oai_password, env):

    #compile eNB (two cases: one for FDD and one for TDD)

    if do_tests(tests['b210']['alu']['fdd']):
        alu = alu_test(epc='amerique', enb='hutch', ue='stevens',
                       openair=openair_dir,
                       user=oai_user, password=oai_password,
                       log_subdir='enb_tests/b210_alu/compile_enb_fdd',
                       env=env)

        try:
            alu.compile_enb("--eNB -w USRP -x -c --disable-cpu-affinity")
        except BaseException, e:
            log("ERROR: ALU test failed: eNB compilation failed: " + str(e))
            return

    if do_tests(tests['b210']['alu']['tdd']):
        alu = alu_test(epc='amerique', enb='calisson', ue='mozart',
                       openair=openair_dir,
                       user=oai_user, password=oai_password,
                       log_subdir='enb_tests/b210_alu/compile_enb_tdd',
                       env=env)

        try:
            alu.compile_enb("--eNB -w USRP -x -c --disable-cpu-affinity")
        except BaseException, e:
            log("ERROR: ALU test failed: eNB compilation failed: " + str(e))
            return

    #compile RRU/RCC eNB

    if do_tests(tests['remote b210']['alu']['fdd']):
        #RRU
        alu = alu_test(epc='amerique', ue='stevens',
                       openair=openair_dir,
                       user=oai_user, password=oai_password,
                       log_subdir='enb_tests/remote_b210_alu/compile_enb_rru',
                       env=env,
                       #hack: set 'enb' to reuse 'compile_enb'
                       enb = 'superserver',
                       enb_rru = 'superserver', enb_rcc = 'starsky')

        try:
            alu.compile_enb("--eNB -x -c -w USRP -t ETHERNET",
                            log_suffix='rru.')
        except BaseException, e:
            log("ERROR: ALU test failed: RRU eNB compilation failed: "+str(e))
            return

        #RCC
        alu = alu_test(epc='amerique', ue='stevens',
                       openair=openair_dir,
                       user=oai_user, password=oai_password,
                       log_subdir='enb_tests/remote_b210_alu/compile_enb_rcc',
                       env=env,
                       #hack: set 'enb' to reuse 'compile_enb'
                       enb = 'starsky',
                       enb_rru = 'superserver', enb_rcc = 'starsky')

        try:
            alu.compile_enb("--eNB -w None -t ETHERNET",
                            log_suffix='rcc.')
        except BaseException, e:
            log("ERROR: ALU test failed: RCC eNB compilation failed: "+str(e))
            return


    #run tests

    udp_dl_bandwidth = { "5"  : "15M",
                         "10" : "30M",
                         "20" : "60M" }

    udp_ul_bandwidth = { "5"  : "7M",
                         "10" : "15M",
                         "20" : "15M" }

    ue_machine = { "fdd" : "stevens",
                   "tdd" : "mozart" }

    enb_machine = { "fdd" : "hutch",
                    "tdd" : "calisson" }

    band = { "fdd" : "7",
             "tdd" : "38" }

    for mode in ('fdd', 'tdd'):
        for bw in ('5', '10', '20'):
            if do_tests(tests['b210']['alu'][mode][bw]):
                log("INFO: ALU test: run tests for bandwidth " + bw + " MHz")
                ctest = tests['b210']['alu'][mode][bw]
                alu = alu_test(epc='amerique', enb=enb_machine[mode], ue=ue_machine[mode],
                               openair=openair_dir,
                               user=oai_user, password=oai_password,
                               log_subdir='enb_tests/b210_alu/' + mode + "/" + bw,
                               env=env)
                try:
                    alu.start_epc()
                    alu.start_enb("enb.band" + band[mode] + ".tm1.usrpb210." + mode + "." + bw + "MHz.conf")
                    for ue in ('bandrich', '3276'):
                        if ue in ctest and do_tests(ctest[ue]):
                            alu.start_ue(ue)
                            if do_tests(ctest[ue]['tcp']['dl']): alu.dl_tcp(ue)
                            if do_tests(ctest[ue]['tcp']['ul']): alu.ul_tcp(ue)
                            if do_tests(ctest[ue]['udp']['dl']): alu.dl_udp(ue, udp_dl_bandwidth[bw])
                            if do_tests(ctest[ue]['udp']['ul']): alu.ul_udp(ue, udp_ul_bandwidth[bw])
                            alu.stop_ue(ue)
                    alu.stop_enb()
                    alu.stop_epc()
                except BaseException, e:
                    log("ERROR: ALU test failed: " + str(e))
                    alu.finish()
            if do_tests(tests['remote b210']['alu'][mode][bw]):
                log("INFO: ALU test: run RRU/RCC tests for bandwidth " + bw + " MHz")
                ctest = tests['remote b210']['alu'][mode][bw]
                alu = alu_test(epc='amerique', enb='', ue=ue_machine[mode],
                               openair=openair_dir,
                               user=oai_user, password=oai_password,
                               log_subdir='enb_tests/remote_b210_alu/' + bw,
                               env=env,
                               enb_rru='superserver', enb_rcc='starsky')
                try:
                    alu.start_epc()
                    alu.start_enb_rru_rcc("rru.band7.tm1.if4p5." + bw + "MHz.udp.usrpb210.conf",
                                          "rcc.band7.tm1.if4p5." + bw + "MHz.conf")
                    for ue in ('bandrich', '3276'):
                        if ue in ctest and do_tests(ctest[ue]):
                            alu.start_ue(ue)
                            if do_tests(ctest[ue]['tcp']['dl']): alu.dl_tcp(ue)
                            if do_tests(ctest[ue]['tcp']['ul']): alu.ul_tcp(ue)
                            if do_tests(ctest[ue]['udp']['dl']): alu.dl_udp(ue, udp_dl_bandwidth[bw])
                            if do_tests(ctest[ue]['udp']['ul']): alu.ul_udp(ue, udp_ul_bandwidth[bw])
                            alu.stop_ue(ue)
                    alu.stop_enb_rru_rcc()
                    alu.stop_epc()
                except BaseException, e:
                    log("ERROR: ALU test failed: " + str(e))
                    alu.finish()
