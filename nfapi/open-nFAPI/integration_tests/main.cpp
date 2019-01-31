/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CUnit.h"
#include "Basic.h"
#include "Automated.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int start_vnf_proc(int port)
{
	char port_str[10];
	sprintf(port_str, "%d", port);

	const char *argv[]  = {"vnfsim", port_str, "../xml/vnf_A.xml", NULL};
	int pid = fork();

	if(pid == 0)
	{
		int result = execv("../vnf_sim/vnfsim", (char* const*)argv); //, env_args);
		if(result == -1)
		{
			printf("Failed to exec vnf process %d\n", errno);
		}
		exit(0);
	}
	else
	{
		return pid;
	}
}

int start_pnf_proc(const char* addr, int port, const char* file)
{
	char port_str[10];
	sprintf(port_str, "%d", port);

	char filename[256];
	sprintf(filename, "../xml/%s", file);

	const char * argv[] = {"pnfsim", addr, port_str, filename, NULL};
	int pid = fork();

	if(pid == 0)
	{
		int result = execv("../pnf_sim/pnfsim", (char* const*)argv); //, env_args);
		if(result == -1)
		{
			printf("Failed to exec pnf process %d\n", errno);
		}
		exit(0);
	}
	else
	{
		return pid;
	}
}

int kill_proc(int pid)
{
	kill(pid, SIGKILL);

	int status;
	int waitpid_result = waitpid(pid, &status, 0);

	return waitpid_result;
}


void test_1()
{

	int pnf_port = 5655;
	const char* pnf_addr = "localhost";

	printf("**** starting vnf *****\n");
	int vnf_pid = start_vnf_proc(pnf_port);

	printf("**** starting pnf *****\n");
	int pnf1_pid = start_pnf_proc(pnf_addr, pnf_port, "pnf_phy_1_A.xml");


	sleep(10);

	printf ("*** Terminating ****\n");
	kill_proc(vnf_pid);
	kill_proc(pnf1_pid);
}
void test_1a()
{

	int pnf_port = 5655;
	const char* pnf_addr = "localhost";

	printf("**** starting vnf *****\n");
	int vnf_pid = start_vnf_proc(pnf_port);

	printf("**** starting pnf *****\n");
	int pnf1_pid = start_pnf_proc(pnf_addr, pnf_port, "pnf_phy_2_A.xml");


	sleep(10);

	printf ("*** Terminating ****\n");
	kill_proc(vnf_pid);
	kill_proc(pnf1_pid);
}


void test_2()
{
	int pnf_port = 5655;
	const char* pnf_addr = "127.0.0.1";

	int vnf_pid = start_vnf_proc(pnf_port);
	sleep(2);

	int pnf1_pid = start_pnf_proc(pnf_addr, pnf_port, "pnf_phy_1_A.xml");	
	int pnf2_pid = start_pnf_proc(pnf_addr, pnf_port, "pnf_phy_1_A.xml");	

	sleep(2);

	kill_proc(pnf2_pid);
	kill_proc(pnf1_pid);
	kill_proc(vnf_pid);
}

void test_32()
{
	int pnf_count = 32;
	int pnf_pid[pnf_count];

	int pnf_port = 5655;
	const char* pnf_addr = "127.0.0.1";

	int vnf_pid = start_vnf_proc(pnf_port);
	sleep(2);

	for(int i = 0; i < pnf_count; ++i)
	{
		pnf_pid[i] = start_pnf_proc(pnf_addr, pnf_port, "pnf_phy_1_A.xml");
	}

	sleep(5);

	printf ("*** Terminating pnfs\n");

	for(int i = 0; i < pnf_count; ++i)
	{
		kill_proc(pnf_pid[i]);
	}
	printf ("*** Terminating vnf\n");
	kill_proc(vnf_pid);
}


int init_suite()
{
	return 0;
}

int clean_suite()
{
	return 0;
}

int main ( void )
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if ( CUE_SUCCESS != CU_initialize_registry() )
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite( "integration_test_suite", init_suite, clean_suite );
   if ( NULL == pSuite ) 
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

        //(NULL == CU_add_test(pSuite, "vnf_test_start_connect_2", vnf_test_start_connect_2)) 
   /* add the tests to the suite */
   if ( (NULL == CU_add_test(pSuite, "test_1a", test_1a))
//        (NULL == CU_add_test(pSuite, "test_2", test_2)) ||
//        (NULL == CU_add_test(pSuite, "test_32", test_32))
      )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   // Run all tests using the basic interface
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_set_output_filename("vnf_unit_test_results.xml");
   CU_basic_run_tests();

	CU_pSuite s = CU_get_registry()->pSuite;
	int count = 0;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

	printf("%d..%d\n", 1, count);



	s = CU_get_registry()->pSuite;
	count = 1;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			int pass = 1;
			CU_FailureRecord* failures = CU_get_failure_list();
			while(failures)
			{
				if(strcmp(failures->pSuite->pName, s->pName) == 0 &&
				   strcmp(failures->pTest->pName, t->pName) == 0)
				{
					pass = 0;
					failures = 0;
				}
				else
				{
					failures = failures->pNext;
				}
			}

			if(pass)
				printf("ok %d - %s:%s\n", count, s->pName, t->pName);
			else 
				printf("not ok %d - %s:%s\n", count, s->pName, t->pName);

			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

   CU_cleanup_registry();
   return CU_get_error();

}
