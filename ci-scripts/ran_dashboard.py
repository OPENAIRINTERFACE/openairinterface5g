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
# Merge Requests Dashboard for RAN on googleSheet 
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------

#import google spreadsheet API
import gspread
from oauth2client.service_account import ServiceAccountCredentials


import subprocess
import shlex      #lexical analysis
import json       #json structures
import datetime   #now() and date formating
from datetime import datetime

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------

class gDashboard:
    def __init__(self, creds_file, spreadsheet, worksheet): #"creds.json", 'OAI RAN Dashboard', 'MR Status'
        self.scope = ["https://spreadsheets.google.com/feeds",'https://www.googleapis.com/auth/spreadsheets',"https://www.googleapis.com/auth/drive.file","https://www.googleapis.com/auth/drive"]
        self.creds = ServiceAccountCredentials.from_json_keyfile_name(creds_file, self.scope)
        self.client = gspread.authorize(self.creds)
        #spreadsheet
        self.ss = self.client.open(spreadsheet)
        #worksheet
        self.sheet = self.ss.worksheet(worksheet)
        self.ss.del_worksheet(self.sheet) #start by deleting the old sheet
        self.sheet = self.ss.add_worksheet(title=worksheet, rows="100", cols="20") #create a new one
        
        self.d = {} #data dictionary


    def fetchData(self,cmd):
        #cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """
        process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        output = process.stdout.readline()
        tmp=output.decode("utf-8") 
        self.d = json.loads(tmp)


    def gBuild(self):

        #line 1 : get update date
        now = datetime.now()
        # dd/mm/YY H:M:S
        dt_string = "Update : " + now.strftime("%d/%m/%Y %H:%M")	
        row =[dt_string]
        self.sheet.insert_row(row, index=1, value_input_option='RAW')

        #line 2 is empty
        #line 3 is column names
        i=3
        row =["MR","Created_at","Author","Title","Assignee", "Reviewer", "CAN START","IN PROGRESS","COMPLETED","OK MERGE","Merge conflicts"]
        self.sheet.insert_row(row, index=i, value_input_option='RAW')

        #line 4 onward, build data lines
        for x in range(len(self.d)):
            i=i+1
            
            
            date_time_str = self.d[x]['created_at']
            date_time_obj = datetime.strptime(date_time_str, '%Y-%m-%dT%H:%M:%S.%fZ')
    
            milestone1=milestone2=milestone3=milestone4=""
            if self.d[x]['milestone']!=None:
                if self.d[x]['milestone']['title']=="REVIEW_CAN_START":
                    milestone1="X"
                elif self.d[x]['milestone']['title']=="REVIEW_IN_PROGRESS":
                    milestone2="X"
                elif self.d[x]['milestone']['title']=="REVIEW_COMPLETED_AND_APPROVED":
                    milestone3="X"
                elif self.d[x]['milestone']['title']=="OK_TO_BE_MERGED": 
                    milestone4="X" 
                else:
                    pass
            else:
                pass

            #check if empty or not
            if self.d[x]['assignee']!=None:
                assignee = str(self.d[x]['assignee']['name'])
            else:
                assignee = ""
 
            #check if empty or not       
            if len(self.d[x]['reviewers'])!=0:
                reviewer = str(self.d[x]['reviewers'][0]['name'])
            else:
                reviewer = ""

            if self.d[x]['has_conflicts']==True:
                conflicts = "YES"
            else:
                conflicts = ""

            #build final row
            row =[str(self.d[x]['iid']), str(date_time_obj.date()),str(self.d[x]['author']['name']),str(self.d[x]['title']),\
            assignee, reviewer,\
            milestone1,milestone2,milestone3,milestone4,conflicts]
            
            #write it to worksheet
            self.sheet.insert_row(row, index=i, value_input_option='RAW')
    
    
    def gFormat(self,sourceSheetName,destinationSheetName):  # "Formating" , "MR Status"
    
        #copy formating template
        sourceSheetId = self.ss.worksheet(sourceSheetName)._properties['sheetId']
        destinationSheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        body = {
            "requests": [
                {
                    "copyPaste": {
                        "source": {
                            "sheetId": sourceSheetId,
                            "startRowIndex": 0,
                            "endRowIndex": 40,
                            "startColumnIndex": 0,
                            "endColumnIndex": 20
                        },
                        "destination": {
                            "sheetId": destinationSheetId,
                            "startRowIndex": 0,
                            "endRowIndex": 40,
                            "startColumnIndex": 0,
                            "endColumnIndex": 20
                        },
                        "pasteType": "PASTE_FORMAT"
                    }
                }
            ]
        }
        self.ss.batch_update(body)    

        #resize columns fit to data, except col 0
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        body = {
            "requests": [
                {
                'autoResizeDimensions': {
                    'dimensions': {
                        'sheetId': sheetId, 
                        'dimension': 'COLUMNS', 
                        'startIndex': 1, 
                        'endIndex': 20
                        }
                    }
                }
            ]   
        }
        self.ss.batch_update(body)


        #resize col 0					
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        body = {
            "requests": [
                {
                    "updateDimensionProperties": {
                        "range": {
                            "sheetId": sheetId,
                            "dimension": "COLUMNS",
                            "startIndex": 0,
                            "endIndex": 1
                        },
                        "properties": {
                            "pixelSize": 100
                        },
                        "fields": "pixelSize"
                    }
                }
            ]
        }
        self.ss.batch_update(body)


        #resize milestones to be cleaner
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        body = {
            "requests": [
                {
                    "updateDimensionProperties": {
                        "range": {
                            "sheetId": sheetId,
                            "dimension": "COLUMNS",
                            "startIndex": 6,
                            "endIndex": 10
                        },
                        "properties": {
                            "pixelSize": 135
                        },
                        "fields": "pixelSize"
                    }
                }
            ]
        }
        self.ss.batch_update(body)
  

def main():
    my_gDashboard=gDashboard("/home/oaicicd/remi/creds.json", 'OAI RAN Dashboard', 'MR Status')
    cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """ 
    my_gDashboard.fetchData(cmd)
    my_gDashboard.gBuild()
    my_gDashboard.gFormat("Formating" , "MR Status")
      
        
if __name__ == "__main__":
    # execute only if run as a script
    main()      
