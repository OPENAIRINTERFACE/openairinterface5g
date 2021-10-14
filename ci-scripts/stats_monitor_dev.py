"""
To create graphs and pickle from runtime statistics in L1,MAC,RRC,PDCP files
"""

import subprocess
import time
import shlex
import re
import sys
import pickle
import matplotlib.pyplot as plt
import numpy as np
import yaml


class StatMonitor():
    def __init__(self,):
        with open('stats_monitor_conf.yaml','r') as file:
            self.d = yaml.load(file)
        for node in self.d:
            for metric in self.d[node]:
                self.d[node][metric]=[]


    def process_gnb (self,node_type,output):
        for line in output:
            tmp=line.decode("utf-8")
            result=re.match(r'^.*\bdlsch_rounds\b ([0-9]+)\/([0-9]+).*\bdlsch_errors\b ([0-9]+)',tmp)
            if result is not None:
                self.d[node_type]['dlsch_err'].append(int(result.group(3)))
                percentage=float(result.group(2))/float(result.group(1))
                self.d[node_type]['dlsch_err_perc_round_1'].append(percentage)
            result=re.match(r'^.*\bulsch_rounds\b ([0-9]+)\/([0-9]+).*\bulsch_errors\b ([0-9]+)',tmp)
            if result is not None:
                self.d[node_type]['ulsch_err'].append(int(result.group(3)))
                percentage=float(result.group(2))/float(result.group(1))
                self.d[node_type]['ulsch_err_perc_round_1'].append(percentage)


    def process_enb (self,node_type,output):
        for line in output:
            tmp=line.decode("utf-8")
            result=re.match(r'^.*\bPHR\b ([0-9]+).+\bbler\b ([0-9]+\.[0-9]+).+\bmcsoff\b ([0-9]+).+\bmcs\b ([0-9]+)',tmp)
            if result is not None:
                self.d[node_type]['PHR'].append(int(result.group(1)))
                self.d[node_type]['bler'].append(float(result.group(2)))
                self.d[node_type]['mcsoff'].append(int(result.group(3)))
                self.d[node_type]['mcs'].append(int(result.group(4)))


    def collect(self,node_type):
        if node_type=='enb':
            cmd='cat L1_stats.log MAC_stats.log PDCP_stats.log RRC_stats.log'
        else: #'gnb'
            cmd='cat nrL1_stats.log nrMAC_stats.log nrPDCP_stats.log nrRRC_stats.log'
        process=subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        output = process.stdout.readlines()
        if node_type=='enb':
            self.process_enb(node_type,output)
        else: #'gnb'
            self.process_gnb(node_type,output)


    def graph(self,node_type):
        col = 1
        figure, axis = plt.subplots(len(self.d[node_type]), col ,figsize=(10, 10))
        i=0
        for metric in self.d[node_type]:
            major_ticks = np.arange(0, len(self.d[node_type][metric])+1, 1)
            axis[i].set_xticks(major_ticks)
            axis[i].set_xticklabels([])
            axis[i].plot(self.d[node_type][metric],marker='o')
            axis[i].set_xlabel('time')
            axis[i].set_ylabel(metric)
            axis[i].set_title(metric)
            i+=1

        plt.tight_layout()
        # Combine all the operations and display
        plt.savefig(node_type+'_stats_monitor.png')
        plt.show()


if __name__ == "__main__":

    node = sys.argv[1]#enb or gnb
    mon=StatMonitor()

    #collecting stats when modem process is stopped
    CMD='ps aux | grep mode | grep -v grep'
    process=subprocess.Popen(CMD, shell=True, stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    while len(output)!=0 :
        mon.collect(node)
        process=subprocess.Popen(CMD, shell=True, stdout=subprocess.PIPE)
        output = process.stdout.readlines()
        time.sleep(1)
    print('Process stopped')
    with open(node+'_stats_monitor.pickle', 'wb') as handle:
        pickle.dump(mon.d, handle, protocol=pickle.HIGHEST_PROTOCOL)
    mon.graph(node)
