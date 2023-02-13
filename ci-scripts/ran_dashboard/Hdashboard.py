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
import shlex
import subprocess
import json       #json structures
import gitlab
import sys

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------

class Dashboard:
    def __init__(self): 
        #init with data sources : git, test results databases
        print("Collecting Data")
        cmd="""curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests?state=opened&per_page=100" """       
        self.git = self.__getGitData(cmd) #git data from Gitlab
        self.mr_list=[] #mr list in string format
        for x in range(len(self.git)):
            self.mr_list.append(str(self.git[x]['iid']))

    def __getGitData(self,cmd):
        process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        output = process.stdout.readline()
        tmp=output.decode("utf-8") 
        d = json.loads(tmp)
        return d

    def PostGitNote(self,mr,commit,args):
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

            body =  '[Consolidated Test Results]\\\n'
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


def main():
    #call from master Jenkinsfile : sh "python3 Hdashboard.py gitpost ${GitPostArgs}"
    if len(sys.argv) > 1:

        #individual MR test results + test dashboard, event based (end of slave jenkins pipeline)
        if sys.argv[1] != "gitpost":
            print("error: only gitpost subcommand is supported")
            exit(1)

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
        else:
            print("Wrong argument at position 1")
            exit(1)


    #test and MR status dashboards, cron based
    else:
        print("error: only gitpost subcommand is supported")
        exit(1)

if __name__ == "__main__":
    # execute only if run as a script
    main()      
