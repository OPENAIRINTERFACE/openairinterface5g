#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import re
import yaml
import os
import sys


def main():
  #read yaml input parameters
  f = open(f'{sys.argv[1]}',)
  data = yaml.full_load(f)
  dir = os.listdir(f'{data[0]["paths"]["source_dir"]}')

  #identify configs, read and replace corresponding values
  for config in data[1]["configurations"]:
    filePrefix = config["filePrefix"]
    outputfilename = config["outputfilename"]
    print('================================================')
    print('filePrefix = ' + filePrefix)
    print('outputfilename = ' + outputfilename)
    for inputfile in dir:
      if inputfile.find(filePrefix) >=0:
        prefix_outputfile = {"cu.band7.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}', 
                             "du.band7.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rru.fdd": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rru.tdd": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "enb.band7.tm1.fr1.25PRB.usrpb210.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "enb.band40.tm1.25PRB.FairScheduler.usrpb210": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band7.tm1.nfapi": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band7.tm1.if4p5.lo.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "rcc.band40.tm1.25PRB": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.band78.tm1.fr1.106PRB.usrpb210.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "gnb.band78.sa.fr1.106PRB.usrpn310.conf": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "ue.nfapi": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}',
                             "ue_sim_ci": f'{data[0]["paths"]["dest_dir"]}/{outputfilename}'
                             }
        print('inputfile = ' + inputfile)
        if filePrefix in prefix_outputfile:
          outputfile1 = prefix_outputfile[filePrefix]  

        directory = f'{data[0]["paths"]["dest_dir"]}'
        if not os.path.exists(directory):
          os.makedirs(directory, exist_ok=True)

        with open(f'{data[0]["paths"]["source_dir"]}{inputfile}', mode='r') as inputfile, \
             open(outputfile1, mode='w') as outputfile:
          for line in inputfile:
            count = 0
            if re.search(r'EHPLMN_LIST', line):
              outputfile.write(line)
              continue
            if re.search(r'sd  = 0x1;', line):
              templine = re.sub(r'sd  = 0x1;', 'sd  = 0x@NSSAI_SD0@;', line)
              outputfile.write(templine)
              continue
            if re.search(r'sd  = 0x112233;', line):
              templine = re.sub(r'sd  = 0x112233;', 'sd  = 0x@NSSAI_SD1@;', line)
              outputfile.write(templine)
              continue
            for key in config["config"]:
              if line.find(key["key"]) >= 0:
                count += 1
                if re.search(r'preference', line):
                  templine = line
                elif re.search(r'mnc_length', line) and key["key"] == "mnc":
                  continue
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
                  templine = re.sub(r'(?<=")[^"]*(?=")', key["env"], line)    
                elif re.search(r'[0-9]', line):
                  templine = re.sub(r'\d+', key["env"], line)
                outputfile.write(templine)
            
            if count == 0:
              outputfile.write(line)
              
if __name__ == "__main__":
    main()
