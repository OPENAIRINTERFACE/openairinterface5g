import subprocess
import time
import shlex
import re
import sys
import matplotlib.pyplot as plt
import pickle
import numpy as np
import os

def collect(d, node_type):
    if node_type=='enb':
        cmd='cat L1_stats.log MAC_stats.log PDCP_stats.log RRC_stats.log'
    else: #'gnb'
        cmd='cat nrL1_stats.log nrMAC_stats.log nrPDCP_stats.log nrRRC_stats.log'
    process=subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    for l in output:
        tmp=l.decode("utf-8")
        result=re.match(rf'^.*\bPHR\b ([0-9]+).+\bbler\b ([0-9]+\.[0-9]+).+\bmcsoff\b ([0-9]+).+\bmcs\b ([0-9]+)',tmp)
        if result is not None:
            d['PHR'].append(int(result.group(1)))
            d['bler'].append(float(result.group(2)))
            d['mcsoff'].append(int(result.group(3)))
            d['mcs'].append(int(result.group(4)))


def graph(d, node_type):


    figure, axis = plt.subplots(4, 1,figsize=(10, 10)) 

    major_ticks = np.arange(0, len(d['PHR'])+1, 1)
    axis[0].set_xticks(major_ticks)
    axis[0].set_xticklabels([])
    axis[0].plot(d['PHR'],marker='o')
    axis[0].set_xlabel('time')
    axis[0].set_ylabel('PHR')
    axis[0].set_title("PHR")
  
    major_ticks = np.arange(0, len(d['bler'])+1, 1)
    axis[1].set_xticks(major_ticks)
    axis[1].set_xticklabels([])
    axis[1].plot(d['bler'],marker='o')
    axis[1].set_xlabel('time')
    axis[1].set_ylabel('bler')
    axis[1].set_title("bler")

    major_ticks = np.arange(0, len(d['mcsoff'])+1, 1)
    axis[2].set_xticks(major_ticks)
    axis[2].set_xticklabels([])
    axis[2].plot(d['mcsoff'],marker='o')
    axis[2].set_xlabel('time')
    axis[2].set_ylabel('mcsoff')
    axis[2].set_title("mcsoff")

    major_ticks = np.arange(0, len(d['mcs'])+1, 1)
    axis[3].set_xticks(major_ticks)
    axis[3].set_xticklabels([])
    axis[3].plot(d['mcs'],marker='o')
    axis[3].set_xlabel('time')
    axis[3].set_ylabel('mcs')
    axis[3].set_title("mcs")

    plt.tight_layout()
    # Combine all the operations and display
    plt.savefig(node_type+'_stats_monitor.png')
    plt.show()

if __name__ == "__main__":

    node_type = sys.argv[1]#enb or gnb

    d={}
    d['PHR']=[]
    d['bler']=[]
    d['mcsoff']=[]
    d['mcs']=[]


    cmd='ps aux | grep mode | grep -v grep'
    process=subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    while len(output)!=0 :
        collect(d, node_type)
        process=subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        output = process.stdout.readlines()
        time.sleep(1)
    print('process stopped')
    with open(node_type+'_stats_monitor.pickle', 'wb') as handle:
        pickle.dump(d, handle, protocol=pickle.HIGHEST_PROTOCOL)
    graph(d, node_type)


