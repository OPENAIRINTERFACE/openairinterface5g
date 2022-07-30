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


#Author Remi
import boto3
import shlex
import subprocess
import json       #json structures
import datetime   #now() and date formating
from datetime import datetime
import re
import gitlab
import yaml
import os
import time
import sys

from sqlconnect import SQLConnect

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------

class Dashboard:
    def __init__(self): 


        #init with data sources : git, yaml config file, test results databases
        print("Collecting Data")
        cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """       
        self.git = self.__getGitData(cmd) #git data from Gitlab
        self.tests = self.__loadCfg('ran_dashboard_cfg.yaml') #tests table setup from yaml
        self.db = self.__loadFromDB() #test results from database
        self.mr_list=[] #mr list in string format
        for x in range(len(self.git)):
            self.mr_list.append(str(self.git[x]['iid']))

    def __loadCfg(self,yaml_file):
        with open(yaml_file,'r') as f:
            tests = yaml.load(f)
        return tests

    def __getGitData(self,cmd):
        #cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """
        process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        output = process.stdout.readline()
        tmp=output.decode("utf-8") 
        d = json.loads(tmp)
        return d

    def __loadFromDB(self):
        mr_list=[] 
        for x in range(len(self.git)):
            mr_list.append(str(self.git[x]['iid']))
        mydb=SQLConnect()
        for MR in mr_list: 
            mydb.get(MR)
        mydb.close_connection()
        return mydb.data

    def singleMR_initHTML(self, date):
        self.f_html.write('<!DOCTYPE html>\n')
        self.f_html.write('<head>\n')
        self.f_html.write('<link rel="stylesheet" href="../test_styles.css">\n')
        self.f_html.write('<title>Test Dashboard</title>\n')
        self.f_html.write('</head>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<table>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Main">OAI RAN TEST Status Dashboard</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<tr></tr>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Date">Update : '+date+'</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('</tr>\n')
        self.f_html.write('</table>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')


    def Test_initHTML(self, date):
        self.f_html.write('<!DOCTYPE html>\n')
        self.f_html.write('<head>\n')
        self.f_html.write('<link rel="stylesheet" href="test_styles.css">\n')
        self.f_html.write('<title>Test Dashboard</title>\n')
        self.f_html.write('</head>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<table>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Main">OAI RAN TEST Status Dashboard</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="DashLink"> <a href="https://oairandashboard.s3.eu-west-1.amazonaws.com/index.html">Merge Requests Dashboard</a></td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('<tr></tr>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Date">Update : '+date+'</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('</tr>\n')
        self.f_html.write('</table>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')

    def Test_terminateHTML(self):
        self.f_html.write('</body>\n')
        self.f_html.write('</html>\n')
        self.f_html.close()


    def MR_initHTML(self,date):
        self.f_html.write('<!DOCTYPE html>\n')
        self.f_html.write('<head>\n')
        self.f_html.write('<link rel="stylesheet" href="mr_styles.css">\n')
        self.f_html.write('<title>MR Dashboard</title>\n')
        self.f_html.write('</head>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<table>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Main">OAI RAN MR Status Dashboard</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="DashLink"> <a href="https://oaitestdashboard.s3.eu-west-1.amazonaws.com/index.html">Tests Dashboard</a></td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('<tr></tr>\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<td class="Date">Update : '+date+'</td>\n')
        self.f_html.write('</td>\n')
        self.f_html.write('</tr>\n')
        self.f_html.write('</table>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<br>\n')
        self.f_html.write('<table class="MR_Table">\n')
        self.f_html.write('<tr>\n')
        self.f_html.write('<th class="MR">MR</th>\n')
        self.f_html.write('<th class="CREATED_AT">Created_At</th>\n')
        self.f_html.write('<th class="AUTHOR">Author</th>\n')
        self.f_html.write('<th class="TITLE">Title</th>\n')
        self.f_html.write('<th class="ASSIGNEE">Assignee</th>\n')
        self.f_html.write('<th class="REVIEWER">Reviewer</th>\n')
        self.f_html.write('<th class="CAN_START">CAN START</th>\n')
        self.f_html.write('<th class="IN_PROGRESS">IN PROGRESS</th>\n')
        self.f_html.write('<th class="COMPLETED">COMPLETED</th>\n')
        self.f_html.write('<th class="REVIEW_FORM">Review Form</th>\n')
        self.f_html.write('<th class="OK_MERGE">OK Merge</th>\n')
        self.f_html.write('<th class="MERGE_CONFLICTS">Merge Conflicts</th>\n')
        self.f_html.write('</tr>\n')

    def MR_terminateHTML(self):
        self.f_html.write('</table> \n')
        self.f_html.write('</body>\n')
        self.f_html.write('</html>\n')
        self.f_html.close()


    def MR_rowHTML(self,row):
        self.f_html.write('<tr>\n')
        self.f_html.write('<td><a href=\"'+row[0]+'\">'+row[1]+'</a></td>\n')
        self.f_html.write('<td>'+row[2]+'</td>\n')
        self.f_html.write('<td>'+row[3]+'</td>\n')
        self.f_html.write('<td class="title_cell">'+row[4]+'</td>\n')
        self.f_html.write('<td>'+row[5]+'</td>\n')
        self.f_html.write('<td>'+row[6]+'</td>\n')
        if row[7]=='X':
            self.f_html.write('<td style="background-color: orange;">'+row[7]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        if row[8]=='X':
            self.f_html.write('<td style="background-color: yellow;">'+row[8]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        if row[9]=='X':
            self.f_html.write('<td style="background-color: rgb(144, 221, 231);">'+row[9]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        if row[10]=='X':
            self.f_html.write('<td style="background-color: rgb(58, 236, 58);">'+row[10]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        if row[11]=='X':
            self.f_html.write('<td style="background-color: rgb(58, 236, 58);">'+row[11]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        if row[12]=='YES':
            self.f_html.write('<td style="background-color: red;">'+row[12]+'</td>\n')
        else:
            self.f_html.write('<td></td>\n')
        self.f_html.write('</tr>\n')


    def Build(self, type, mr, htmlfilename):
        if type=='MR':
            self.Build_MR_Table(htmlfilename)
        elif type=='Tests':
            self.Build_Test_Table(htmlfilename)
        elif type=='singleMR':
            self.Build_singleMR_Table(mr,htmlfilename)
        else :
            print("Undefined Dashboard Type, options : MR or Tests")


    def Build_Test_Table(self,htmlfilename):
        print("Building Tests Dashboard...")

        self.f_html=open(htmlfilename,'w')

        ###update date/time, format dd/mm/YY H:M:S
        now = datetime.now()
        dt_string = now.strftime("%d/%m/%Y %H:%M")	  
        #HTML table header
        self.Test_initHTML(dt_string)


        #1 table per MR if test results exist
        for x in range(len(self.git)):
            mr=str(self.git[x]['iid'])
            if 'PASS' not in self.db[mr]:
                self.f_html.write('<h3><a href="https://gitlab.eurecom.fr/oai/openairinterface5g/-/merge_requests/'+mr+'">'+mr+'</a>'+'   '+self.git[x]['title'] + '</h3>\n')
                self.f_html.write('<table class="Test_Table">\n')
                self.f_html.write('<tr>\n')
                self.f_html.write('<th class="Test_Name">Test Name</th>\n')
                self.f_html.write('<th class="Test_Descr">Bench</th> \n')  
                self.f_html.write('<th class="Test_Descr">Test</th> \n')
                self.f_html.write('<th class="Pass"># Pass</th>\n')
                self.f_html.write('<th class="Fail"># Fail</th>\n')
                self.f_html.write('<th class="Last_Pass">Last Pass</th>\n')
                self.f_html.write('<th class="Last_Fail">Last Fail</th>\n')
                self.f_html.write('</tr>\n')

                #parsing the tests
                for t in self.tests:

                    row=[]
                    short_name= t
                    hyperlink= self.tests[t]['link']
                    job=self.tests[t]['job']


                    if job in self.db[mr]:
                        if 'PASS' in self.db[mr][job]:
                            row.append(self.db[mr][job]['PASS'])
                        else:
                            row.append('')
                        if 'FAIL' in self.db[mr][job]:
                            row.append(self.db[mr][job]['FAIL'])
                        else:
                            row.append('')
                        #2 columns for last_pass and last_fail links
                        if 'last_pass' in self.db[mr][job]:
                            lastpasshyperlink=  self.db[mr][job]['last_pass'][1]
                            lastpasstext= self.db[mr][job]['last_pass'][0]
                        else:
                            lastpasshyperlink=''
                            lastpasstext=''

                        if 'last_fail' in self.db[mr][job]:
                            lastfailhyperlink=  self.db[mr][job]['last_fail'][1] 
                            lastfailtext= self.db[mr][job]['last_fail'][0]
                        else:
                            lastfailhyperlink=''
                            lastfailtext=''



                        self.f_html.write('<tr>\n')
                        self.f_html.write('<td><a href='+hyperlink+'>'+short_name+'</a></td>\n')
                        self.f_html.write('<td>'+self.tests[t]['bench']+'</td>\n')
                        self.f_html.write('<td>'+self.tests[t]['test']+'</td>\n')
                        if row[0]!='':
                            self.f_html.write('<td style="background-color: rgb(58, 236, 58);">'+str(row[0])+'</td>\n')
                        else:
                            self.f_html.write('<td></td>\n')
                        if row[1]!='':
                            self.f_html.write('<td style="background-color: red;">'+str(row[1])+'</td>\n')
                        else:
                            self.f_html.write('<td></td>\n')
                        self.f_html.write('<td><a href='+lastpasshyperlink+'>'+lastpasstext+'</a></td>\n')
                        self.f_html.write('<td><a href='+lastfailhyperlink+'>'+lastfailtext+'</a></td>\n')
                        self.f_html.write('</tr>\n')

                self.f_html.write('</table>\n')

        #terminate HTML table and close file
        self.Test_terminateHTML()


    def Build_singleMR_Table(self,singlemr,htmlfilename):
        print("Building single MR Tests Results...")

        self.f_html=open(htmlfilename,'w')

        ###update date/time, format dd/mm/YY H:M:S
        now = datetime.now()
        dt_string = now.strftime("%d/%m/%Y %H:%M")	  
        #HTML table header
        self.singleMR_initHTML(dt_string)


        #1 table per MR if test results exist => 1 table for matching mr
        for x in range(len(self.git)):
            mr=str(self.git[x]['iid'])
            if mr==singlemr:
            #if 'PASS' not in self.db[mr]:
                self.f_html.write('<h3><a href="https://gitlab.eurecom.fr/oai/openairinterface5g/-/merge_requests/'+mr+'">'+mr+'</a>'+'   '+self.git[x]['title'] + '</h3>\n')
                self.f_html.write('<table class="Test_Table">\n')
                self.f_html.write('<tr>\n')
                self.f_html.write('<th class="Test_Name">Test Name</th>\n')
                self.f_html.write('<th class="Test_Descr">Bench</th> \n')  
                self.f_html.write('<th class="Test_Descr">Test</th> \n')
                self.f_html.write('<th class="Pass"># Pass</th>\n')
                self.f_html.write('<th class="Fail"># Fail</th>\n')
                self.f_html.write('<th class="Last_Pass">Last Pass</th>\n')
                self.f_html.write('<th class="Last_Fail">Last Fail</th>\n')
                self.f_html.write('</tr>\n')

                #parsing the tests
                for t in self.tests:

                    row=[]
                    short_name= t
                    hyperlink= self.tests[t]['link']
                    job=self.tests[t]['job']


                    if job in self.db[mr]:
                        if 'PASS' in self.db[mr][job]:
                            row.append(self.db[mr][job]['PASS'])
                        else:
                            row.append('')
                        if 'FAIL' in self.db[mr][job]:
                            row.append(self.db[mr][job]['FAIL'])
                        else:
                            row.append('')
                        #2 columns for last_pass and last_fail links
                        if 'last_pass' in self.db[mr][job]:
                            lastpasshyperlink=  self.db[mr][job]['last_pass'][1]
                            lastpasstext= self.db[mr][job]['last_pass'][0]
                        else:
                            lastpasshyperlink=''
                            lastpasstext=''

                        if 'last_fail' in self.db[mr][job]:
                            lastfailhyperlink=  self.db[mr][job]['last_fail'][1] 
                            lastfailtext= self.db[mr][job]['last_fail'][0]
                        else:
                            lastfailhyperlink=''
                            lastfailtext=''



                        self.f_html.write('<tr>\n')
                        self.f_html.write('<td><a href='+hyperlink+'>'+short_name+'</a></td>\n')
                        self.f_html.write('<td>'+self.tests[t]['bench']+'</td>\n')
                        self.f_html.write('<td>'+self.tests[t]['test']+'</td>\n')
                        if row[0]!='':
                            self.f_html.write('<td style="background-color: rgb(58, 236, 58);">'+str(row[0])+'</td>\n')
                        else:
                            self.f_html.write('<td></td>\n')
                        if row[1]!='':
                            self.f_html.write('<td style="background-color: red;">'+str(row[1])+'</td>\n')
                        else:
                            self.f_html.write('<td></td>\n')
                        self.f_html.write('<td><a href='+lastpasshyperlink+'>'+lastpasstext+'</a></td>\n')
                        self.f_html.write('<td><a href='+lastfailhyperlink+'>'+lastfailtext+'</a></td>\n')
                        self.f_html.write('</tr>\n')

                self.f_html.write('</table>\n')

        #terminate HTML table and close file
        self.Test_terminateHTML()



    def Build_MR_Table(self,htmlfilename):

        print("Building Merge Requests Dashboard...")

        self.f_html=open(htmlfilename,'w')

        ###update date/time, format dd/mm/YY H:M:S
        now = datetime.now()
        dt_string = now.strftime("%d/%m/%Y %H:%M")	  

        #HTML table header
        self.MR_initHTML(dt_string)


        ###MR data lines
        for x in range(len(self.git)):


            hyperlink= 'https://gitlab.eurecom.fr/oai/openairinterface5g/-/merge_requests/'+ str(self.git[x]['iid'])
            text= str(self.git[x]['iid'])

                      
            date_time_str = self.git[x]['created_at']
            date_time_obj = datetime.strptime(date_time_str, '%Y-%m-%dT%H:%M:%S.%fZ')
    
            milestone1=milestone2=milestone3=milestone4=""
            if self.git[x]['milestone']!=None:
                if self.git[x]['milestone']['title']=="REVIEW_CAN_START":
                    milestone1="X"
                elif self.git[x]['milestone']['title']=="REVIEW_IN_PROGRESS":
                    milestone2="X"
                elif self.git[x]['milestone']['title']=="REVIEW_COMPLETED_AND_APPROVED":
                    milestone3="X"
                elif self.git[x]['milestone']['title']=="OK_TO_BE_MERGED": 
                    milestone4="X" 
                else:
                    pass
            else:
                pass

            #check if empty or not
            if self.git[x]['assignee']!=None:
                assignee = str(self.git[x]['assignee']['name'])
            else:
                assignee = ""
 
            #check if empty or not       
            if len(self.git[x]['reviewers'])!=0:
                reviewer = str(self.git[x]['reviewers'][0]['name'])
            else:
                reviewer = ""

            if self.git[x]['has_conflicts']==True:
                conflicts = "YES"
            else:
                conflicts = ""


            #add a column flagging that the review form is present
            #we use gitlab API to parse the MR notes
            gl = gitlab.Gitlab.from_config('OAI')
            project_id = 223
            project = gl.projects.get(project_id)
            #get the opened MR in the project
            mrs = project.mergerequests.list(state='opened',per_page=100)
            review_form=''
            for m in range (0,len(mrs)):
                if mrs[m].iid==self.git[x]['iid']:#check the iid is the one we are on
                    mr_notes = mrs[m].notes.list(all=True)
                    n=0
                    found=False
                    while found==False and n<len(mr_notes):
                        res=re.search('Code Review by',mr_notes[n].body)#this is the marker we are looking for in all notes
                        if res!=None:
                            review_form = "X"
                            found=True
                        n+=1

            #build final row to be inserted
            row =[hyperlink, text, str(date_time_obj.date()),str(self.git[x]['author']['name']), str(self.git[x]['title']),\
            assignee, reviewer,\
            milestone1,milestone2,milestone3,review_form,milestone4,conflicts]

            self.MR_rowHTML(row)

        #terminate HTML table and close file
        self.MR_terminateHTML()


    def CopyToS3(self,htmlfilename,bucket,key):
        print("Uploading to S3 bucket")
        #Creating Session With Boto3.
        s3 = boto3.client('s3')

        #Creating S3 Resource From the Session.
        result = s3.upload_file(htmlfilename, bucket,key, ExtraArgs={'ACL':'public-read','ContentType': 'text/html'})

    #unused
    def CopyCSS(self,path):
        s3 = boto3.resource('s3')
        copy_source = {'Bucket': 'oaitestdashboard','Key':'test_styles.css'}
        s3.meta.client.copy(copy_source, 'oaitestdashboard', path+'/'+ 'test_styles.css')


    def PostGitNote(self,mr,commit,args):
        #current date and time to be posted with test results
        #now = datetime.now()
        #dt_string = now.strftime("%d/%m/%Y %H:%M")

        if len(args)%4 != 0:
            print("Wrong Number of Arguments")
            return
        else :
            n_tests=len(args)//4

            gl = gitlab.Gitlab.from_config('OAI')
            project_id = 223
            project = gl.projects.get(project_id)

            #retrieve all the notes from the MR
            editable_mr = project.mergerequests.get(int(mr))
            mr_notes = editable_mr.notes.list(all=True)

            body =  '[Consolidated Test Results](https://oaitestdashboard.s3.eu-west-1.amazonaws.com/MR'+mr+'/index.html)\\\n'
            body += 'Tested CommitID: ' + commit

            for i in range(0,n_tests):
                jobname = args[4*i]
                buildurl = args[4*i+1]
                buildid = args[4*i+2]
                status = args[4*i+3]
                body += '\\\n' + jobname + ': **'+status+'** ([' + buildid + '](' + buildurl + '))'

            #create new note
            mr_note = editable_mr.notes.create({
                'body': body
            })
            editable_mr.save()

    def AWSCleanup(self,mode):
        #first build MR list from aws S3 bucket
        if mode != 'report' and mode !='delete':
            print("incorrect mode for awsclean")
            return
        aws_mr_list=[]
        s3 = boto3.resource('s3')
        my_bucket = s3.Bucket('oaitestdashboard')
        for my_bucket_object in my_bucket.objects.all():
            #MR objects are like MR1407/index.html
            res=re.search(r'^MR([0-9]+)',my_bucket_object.key)
            if res!=None:
                aws_mr_list.append(res.group(1))#store MR number as a string
        #open MR list from GIt already exists as an attribute of this class self.mr_list
        #parse aws MR list and delete those MR that are no longer open        
        for aws_mr in aws_mr_list:
            if aws_mr not in self.mr_list:
                if mode=="report":
                    print(aws_mr+' can be deleted from AWS S3')
                else :
                    awspath="MR"+aws_mr+"/"
                    print('deleting ' + aws_mr)
                    my_bucket.objects.filter(Prefix=awspath).delete()




def main():

    #call from slave Jenkinsfile : sh "python3 Hdashboard.py testevent ${params.eNB_MR} "  
    #call from master Jenkinsfile : sh "python3 Hdashboard.py gitpost ${GitPostArgs}"
   

    if len(sys.argv)>1:

        #individual MR test results + test dashboard, event based (end of slave jenkins pipeline)
        if sys.argv[1]=="testevent" :
            mr=sys.argv[2]
            htmlDash=Dashboard()
            if mr in htmlDash.mr_list:
                #single MR test results
                htmlDash.Build('singleMR',mr,'/tmp/MR'+mr+'_index.html') 
                htmlDash.CopyToS3('/tmp/MR'+mr+'_index.html','oaitestdashboard','MR'+mr+'/index.html')
                #all MR test results
                htmlDash.Build('Tests','0000','/tmp/Tests_index.html') 
                htmlDash.CopyToS3('/tmp/Tests_index.html','oaitestdashboard','index.html')

        #git post with MR test results, event based (end of master jenkins pipeline)
        elif sys.argv[1]=="gitpost":
            mr=sys.argv[2]
            commit=sys.argv[3]
            args=[]
            for i in range (4, len(sys.argv)): #jobname, url, id , result
                args.append(sys.argv[i])
            htmlDash=Dashboard()
            if mr in htmlDash.mr_list:
                htmlDash.PostGitNote(mr,commit, args)
            else:
                print("Not a Merge Request => this build is for testing/debug purpose, no report to git")
        elif sys.argv[1]=="awsclean":
            mode=sys.argv[2]#report or delete
            htmlDash=Dashboard()
            htmlDash.AWSCleanup(mode)
        else:
            print("Wrong argument at position 1")


    #test and MR status dashboards, cron based
    else:
        htmlDash=Dashboard()
        #all MR status dashboard
        htmlDash.Build('MR','0000','/tmp/MR_index.html') 
        htmlDash.CopyToS3('/tmp/MR_index.html','oairandashboard','index.html') 
        #all MR test results
        htmlDash.Build('Tests','0000','/tmp/Tests_index.html') 
        htmlDash.CopyToS3('/tmp/Tests_index.html','oaitestdashboard','index.html') 



if __name__ == "__main__":
    # execute only if run as a script
    main()      
