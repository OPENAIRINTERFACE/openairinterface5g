/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*________________________OEPNAIR/OPENAIR0/________________________

 File    : Readme.txt 
 Authors : navid nikaein
 Company : EURECOM
 Emails  : navid.nikaein@eurecom.fr
________________________________________________________________*/


-------------------------
Table of contents
-------------------------

The content of this readme is the following: 

  1) What is this block about in OpenAirInterface
  2) Folders and files description
  3) Organization of the folders and their dependancies
  4) Makefile targets and how to build
  5) How to use through a tutorial: run a simple experimentation

------------------------------------------------
1) What is this block about in OpenAirInterface ?
-----------------------------------------------
     

     It essentially means "the hardware part" located at openair0.

     More information about ???  can be found on the Twiki:
     https://twiki.eurecom.fr/twiki/bin/view/OpenAirInterface/WebHome

-----------------------------------------
2) Folders and files description
-----------------------------------------

   File/Folder        Description
    -----------        -----------
    
    - Folder1/       contains 
    
    - Folder2/       contains 
    
    - init.bash          A simple script file, that any user SHOULD source from 
    		       its environment personal set-up script (this is typically ~/.bashrc) 
		       by adding the following lines to it:

                        # This is my ~/.bashrc file or equivalent
                         export OPENAIRX=path/to/my/openairX/folder
                         source $OPENAIRX/init.bash
    
                       The file includes variable definitions & path settings to
                       access softwares, sources & Makefiles, software
                       distribution in openair, and so on.

                       YOU DEFINETELY NEED TO SOURCE THIS FILE.

----------------------------------------------------
3) Organization of the folders and their dependancies
----------------------------------------------------

     The 3 folders have the following structures

     - Folder1/subfolder1/  contains any ressource related to the design

     - Folder2/subfolder2/ contains 

     - Folder2/src/ 

     - Folder2/lib/ 

     - Folder2/bin/ 

      Explain where are the source files.  
      Explain the relationship with other Blocks.

    
-----------------------------------------
4)  Makefile targets and how to build
-----------------------------------------

     Explain whether you are using symbolic links or not,
     how to backup/archive
     how to generate the tags
     how to print the vars
     how to create the documentation 

----------------------------------------------------------------
6)  How to use through a tutorial: run a simple experimentation
----------------------------------------------------------------

      ...

