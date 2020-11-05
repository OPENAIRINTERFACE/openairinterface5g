import re
import json
import os


dir = os.listdir('/home/mohammed/Documents/conf_files/')

#read json config file
f = open('parameters.json',)
data = json.load(f)
for config in data:

  filePrefix = config["filePrefix"]
  print(filePrefix)
  for inputfile in dir:
    if inputfile.find(filePrefix) >=0:
      if filePrefix == "cu":
        outputfile1 = 'config/cu.fdd.config'
      elif filePrefix == "du": 
        outputfile1 = 'config/du.fdd.config'
      elif filePrefix == "du": 
        outputfile1 = 'config/du.fdd.config'
      elif filePrefix == "rru.fdd": 
        outputfile1 = 'config/rru.fdd.config'     
      elif filePrefix == "rru.tdd": 
        outputfile1 = 'config/rru.tdd.config'
      elif filePrefix == "enb.band7.tm1.25PRB.usrpb210": 
        outputfile1 = 'config/enb.fdd.config'
      elif filePrefix == "enb.band40.tm1.25PRB.FairScheduler.usrpb210": 
        outputfile1 = 'config/enb.tdd.config'   
#      elif filePrefix == "rcc.band7.tm1.nfapi": 
#        outputfile1 = 'config/rcc.if4p5.enb.fdd.config'
#      elif filePrefix == "rcc.band7.tm1.nfapi": 
#        outputfile1 = 'config/rcc.if4p5.enb.tdd.config' 
      
      directory = 'config/'
      if not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)
  
      with open("/home/mohammed/Documents/conf_files/%s" % (inputfile), mode='r') as inputfile, \
           open(outputfile1, mode='w') as outputfile:
        for line in inputfile:
          count = 0
          for key in config["config"]:
            if line.find(key["key"]) >= 0:
              count += 1
              if re.search(r'preference', line):
                templine = line
              elif re.search(r'plmn_list', line):
                templine = re.sub(r'[0-9]+', '""', line)
                templine = re.sub(r'\"\"', key["env"]["mcc"], templine, 1)
                templine = re.sub(r'\"\"', key["env"]["mnc"], templine, 1) 
                templine = re.sub(r'\"\"', key["env"]["mnc_length"], templine, 1)               
              elif re.search('downlink_frequency', line):
                templine = re.sub(r'[0-9]+', key["env"], line)
              elif re.search('uplink_frequency_offset', line):
                templine = re.sub(r'[0-9]+', key["env"], line)
               
              elif re.search(r'"(.*?)"', line):
                templine = re.sub(r'(?<=")[^"]*(?=")', key["env"], line)      # for quotes     
              elif re.search(r'[0-9]', line):
                templine = re.sub(r'\d+', key["env"], line)
              outputfile.write(templine)
            
          if count == 0:
            outputfile.write(line)
#read file and replace with ...

