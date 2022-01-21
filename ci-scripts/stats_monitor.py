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
    def __init__(self,cfg_file):
        with open(cfg_file,'r') as file:
            self.d = yaml.load(file)
        for node in self.d:#so far we have enb or gnb as nodes
            for metric_l1 in self.d[node]: #first level of metric keys
                if metric_l1!="graph": #graph is a reserved word to configure graph paging, so it is disregarded
                    if self.d[node][metric_l1] is None:#first level is None -> create array
                        self.d[node][metric_l1]=[]
                    else: #first level is not None -> there is a second level -> create array
                        for metric_l2 in self.d[node][metric_l1]:
                            self.d[node][metric_l1][metric_l2]=[]                



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

            for k in self.d[node_type]['rt']:
                result=re.match(rf'^.*\b{k}\b:\s+([0-9\.]+) us;\s+([0-9]+);\s+([0-9\.]+) us;',tmp)
                if result is not None:
                    self.d[node_type]['rt'][k].append(float(result.group(3)))


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
            files = ["L1_stats.log", "MAC_stats.log", "PDCP_stats.log", "RRC_stats.log"]
        else: #'gnb'
            files = ["nrL1_stats.log", "nrMAC_stats.log", "nrPDCP_stats.log", "nrRRC_stats.log"]
        # append each file's contents to another file (prepended with CI-) for
        # post-mortem/debugging analysis
        for f in files:
            cmd = rf'cat {f} >> CI-{f}'
            subprocess.Popen(shlex.split(cmd))
        # join the files for further processing
        cmd = rf'cat {shlex.join(files)}'
        process=subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        output = process.stdout.readlines()
        if node_type=='enb':
            self.process_enb(node_type,output)
        else: #'gnb'
            self.process_gnb(node_type,output)


    def graph(self,node_type):
        for page in self.d[node_type]['graph']:#work out a set a graphs per page
            col = 1
            figure, axis = plt.subplots(len(self.d[node_type]['graph'][page]), col ,figsize=(10, 10))
            i=0
            for m in self.d[node_type]['graph'][page]:#metric may refer to 1 level or 2 levels 
                metric_path=m.split('.')
                if len(metric_path)==1:#1 level
                    metric_l1=metric_path[0]
                    major_ticks = np.arange(0, len(self.d[node_type][metric_l1])+1, 1)
                    axis[i].set_xticks(major_ticks)
                    axis[i].set_xticklabels([])
                    axis[i].plot(self.d[node_type][metric_l1],marker='o')
                    axis[i].set_xlabel('time')
                    axis[i].set_ylabel(metric_l1)
                    axis[i].set_title(metric_l1)
                    
                else:#2 levels
                    metric_l1=metric_path[0]
                    metric_l2=metric_path[1]
                    major_ticks = np.arange(0, len(self.d[node_type][metric_l1][metric_l2])+1, 1)
                    axis[i].set_xticks(major_ticks)
                    axis[i].set_xticklabels([])
                    axis[i].plot(self.d[node_type][metric_l1][metric_l2],marker='o')
                    axis[i].set_xlabel('time')
                    axis[i].set_ylabel(metric_l2)
                    axis[i].set_title(metric_l2)
                i+=1                

            plt.tight_layout()
            #save as png
            plt.savefig(node_type+'_stats_monitor_'+page+'.png')


if __name__ == "__main__":

    cfg_filename = sys.argv[1] #yaml file as metrics config
    node = sys.argv[2]#enb or gnb
    mon=StatMonitor(cfg_filename)

    #collecting stats when modem process is stopped
    CMD='ps aux | grep modem | grep -v grep'
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
