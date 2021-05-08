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
import re
import gitlab

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


    def gBuild(self, destinationSheetName):

        #line 1 : update date/time, format dd/mm/YY H:M:S
        now = datetime.now()
        dt_string = "Update : " + now.strftime("%d/%m/%Y %H:%M")	
        row =[dt_string]
        self.sheet.insert_row(row, index=1, value_input_option='RAW')

        #line 2 empty
        #line 3 is for the column names
        i=3
        row =["MR","Created_at","Author","Title","Assignee", "Reviewer", "CAN START","IN PROGRESS","COMPLETED","Review Form","OK MERGE","Merge conflicts"]
        self.sheet.insert_row(row, index=i, value_input_option='RAW')

        #line 4 onward, MR data lines
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


            #add a column flagging that the review form is present
            #we use gitlab API to parse the MR notes
            gl = gitlab.Gitlab.from_config('OAI')
            project_id = 223
            project = gl.projects.get(project_id)
            #get the opened MR in the project
            mrs = project.mergerequests.list(state='opened')
            for m in range (0,len(mrs)):
                if mrs[m].iid==self.d[x]['iid']:#check the iid is the one we are on
                    mr_notes = mrs[m].notes.list(all=True)
                    n=0
                    found=False
                    review_form=""
                    while found==False and n<len(mr_notes):
                        res=re.search('Code Review by',mr_notes[n].body)#this is the marker we are looking for in all notes
                        if res!=None:
                            review_form = "X"
                            found=True
                        n+=1


            #build final row to be inserted, the first column is left empty for now, will be filled afterward with hyperlinks to gitlab MR
            row =["", str(date_time_obj.date()),str(self.d[x]['author']['name']),str(self.d[x]['title']),\
            assignee, reviewer,\
            milestone1,milestone2,milestone3,review_form,milestone4,conflicts]
            
            #insert the row to worksheet
            self.sheet.insert_row(row, index=i, value_input_option='RAW')
        
        
        #add MR hyperlinks in a list of requests to be sent as one update batch; this to save API calls (quotas) 
        i=3
        requests=[]
        for x in range(len(self.d)):              
            rowIndex=i
            colIndex=0
            hyperlink= '\"'+"https://gitlab.eurecom.fr/oai/openairinterface5g/-/merge_requests/"+ str(self.d[x]['iid']) +'\"'
            text= '\"'+str(self.d[x]['iid'])+'"'
            requests.append(self.addHyperlink(hyperlink, text, destinationSheetName, rowIndex, colIndex))
            i=i+1
        body = {"requests": requests}    
        self.ss.batch_update(body)        
            

    
    def addHyperlink(self, hyperlink, text, destinationSheetName, rowIndex, colIndex):
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        request =\
                {
                "updateCells": {
                        "rows": [
                            {
                            "values": [
                                {
                                "userEnteredValue": {
                                "formulaValue":"=HYPERLINK({},{})".format(hyperlink, text) 
                                }
                                }
                            ]
                            }
                        ],
                        "fields": "userEnteredValue",
                        "start": {
                            "sheetId": sheetId,
                            "rowIndex": rowIndex,
                            "columnIndex": colIndex
                        }
                }
              }
        return request


    
    def gFormat(self,sourceSheetName,destinationSheetName):  # "Formating" , "MR Status"
        #the requests are appended in a list of requests to be sent as one update batch; this to save API calls (quotas) 
        #copy formating template
        sourceSheetId = self.ss.worksheet(sourceSheetName)._properties['sheetId']
        destinationSheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        requests=[]
        requests.append(
                {
                    "copyPaste": {
                        "source": {
                            "sheetId": sourceSheetId,
                            "startRowIndex": 0,
                            "endRowIndex": 40,
                            "startColumnIndex": 0,
                            "endColumnIndex": 12
                        },
                        "destination": {
                            "sheetId": destinationSheetId,
                            "startRowIndex": 0,
                            "endRowIndex": 40,
                            "startColumnIndex": 0,
                            "endColumnIndex": 12
                        },
                        "pasteType": "PASTE_FORMAT"
                    }
                }
        )
        

        #resize columns fit to data, except col 0
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        requests.append(
                {
                'autoResizeDimensions': {
                    'dimensions': {
                        'sheetId': sheetId, 
                        'dimension': 'COLUMNS', 
                        'startIndex': 1, 
                        'endIndex': 12
                        }
                    }
                }   
        )


        #resize col 0					
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        requests.append(
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
        )


        #resize milestones to be cleaner
        sheetId = self.ss.worksheet(destinationSheetName)._properties['sheetId']
        requests.append(
                {
                    "updateDimensionProperties": {
                        "range": {
                            "sheetId": sheetId,
                            "dimension": "COLUMNS",
                            "startIndex": 6,
                            "endIndex": 11
                        },
                        "properties": {
                            "pixelSize": 120
                        },
                        "fields": "pixelSize"
                    }
                }
        )
    
        body = {"requests": requests}    
        self.ss.batch_update(body)
  


def main():
    my_gDashboard=gDashboard("/home/oaicicd/remi/creds.json", 'OAI RAN Dashboard', 'MR Status')
    cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """ 
    my_gDashboard.fetchData(cmd)
    my_gDashboard.gBuild("MR Status")
    my_gDashboard.gFormat("Formating" , "MR Status")

      
        
if __name__ == "__main__":
    # execute only if run as a script
    main()      
