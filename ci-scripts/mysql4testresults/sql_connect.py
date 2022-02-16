import pymysql
import sys
from datetime import datetime

#This is the script used to write the test results to the mysql DB
#Called by Jenkins pipeline (jenkinsfile)
#Must be located in /home/oaicicid/mysql on the database host
#Usage from Jenkinsfile : 
#python3 /home/oaicicd/mysql/sql_connect.py ${JOB_NAME} ${params.eNB_MR} ${params.eNB_Branch} ${env.BUILD_ID} ${env.BUILD_URL} ${StatusForDb} ''

class SQLConnect:
    def __init__(self):
        self.connection = pymysql.connect(
                host='172.22.0.2',
                user='root', 
                password = 'ucZBc2XRYdvEm59F',
                db='oaicicd_tests',
                port=3306
                )

    def put(self,TEST,MR,BRANCH,BUILD,BUILD_LINK,STATUS):
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        cur=self.connection.cursor()
        cur.execute ('INSERT INTO test_results (TEST,MR,BRANCH,BUILD,BUILD_LINK,STATUS,DATE) VALUES (%s,%s,%s,%s,%s,%s,%s);' , (TEST, MR, BRANCH, BUILD, BUILD_LINK, STATUS, now))
        self.connection.commit()
        self.connection.close()


if __name__ == "__main__":
    mydb=SQLConnect()
    mydb.put(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6])


