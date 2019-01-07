-----------------------------------------
1) Folders and files description
-----------------------------------------

This directory contains different targets for OpenAirInterface.org implementations

ARCH/EXMIMO - Kernel driver for EXMIMO1+2 
RTAI/USER   - Target to run .11p (uses openairITS) or LTE modem (uses openair1 and openair2) for x86 in RTAI user-space
SIMU    - Simulator/Emulator target for x86
DOCS    - Doxygen documentation generation for openair1/openair2


First, you must have all four openair SW directories, openair1, openair2, openair3, and targets at the same hierarchical level as this directory.

1. The following packages must be available on your system:

        - ssh, python, python pexpect 
        - libblas and libblas-dev
        - libconfig++8-dev
        - libgtk-3-dev
        - libpgm-5.1 and libpgm-5.1-dev for distributed simulation with reliable multicast transmport
        - libxml2 and libxml2-dev
        - libforms-bin libforms-dev
        - nettle-dev nettle-bin openssl libssl-dev
        - libatlas-base-dev and libatlas-headers (for Ubuntu 11.04, libatlas-dev instead of libatlas-headers)
        - asn1c and the LTE ASN1 files (for more information on how to install this and generate the required files see the README file in openair2/RRC/LITE/MESSAGES) 

----------------------------------------------------
2) Organization of the folders and their dependancies
----------------------------------------------------
The 3 folders have the following structures
   
    - EXAMPLES: there you can find some basic examples with/without the network interface
    - KERN: this folder includes the files for kernel space compilation targeting realtime operation (RF/emulation platform)
    - USER: this folder includes the files for user space compilation targeting soft realtime operation (emulation/simulation/debugging)

----------------------------------------------------------------
3)  How to use through a tutorial: run a simple experimentation
----------------------------------------------------------------
1. Up to know you should have created a directory: mkdir openair4G

2. Check out the openair4G/trunk repository:  svn co https://svn.eurecom.fr/openairsvn/openair4G/trunk openair4G

3. Now, please check the following:
   
   - the env variables in .bashrc the following lines, and source them if necessary :
    set OPENAIR_DIR variable and export the following:
    export OPENAIR1_DIR=$OPENAIR_DIR/openair4G/openair1
    export OPENAIR2_DIR=$OPENAIR_DIR/openair4G/openair2
    export OPENAIR3_DIR=$OPENAIR_DIR/openair4G/openair3
    export OPENAIRITS_DIR=$OPENAIR_DIR/openair4G/openairITS
    export OPENAIR_TARGETS=$OPENAIR_DIR/openair4G/targets/
    
   - checkout and compile the asn1 messages 
     follow the README.txt in OPENAIR2_DIR/RRC/LITE/MESSAGES to checkout the ASN.1 RRC messages
     follow the OPENAIR2_DIR/RRC/LITE/MESSAGE/asn1c/asn1c/INSTALL to install the asn1c
     go back to $OPENAIR_TARGET and "make install" if you have not used the shell script to install asn1c 
   - cd $OPENAIR_TARGET/SIMU/USER, and make a sanity check by doing make pre-ci (require 15 minutes)
   - Check out simple examples in $OPENAIR_TARGET/SIMU/EXAMPLES 

4. to automatically bring up the oai interfaces and configure the default radio bearer (useful when sending data traffic):
   - add your username into the list of sudoers
     + edit /etc/sudoers as a super user 
     + after the line: # User privilege specification, add : user_name   ALL=(ALL:ALL) ALL
     + after the line: %admin ALL=(ALL) ALL,           add : user_name   ALL=(ALL) NOPASSWD: ALL
   - include this line: alias sudo='sudo -E', to the .bashrc 
   - run oai as follows: sudo ./oaisim arg1 arg2 ....


----------------------------------------------------------------
4)  Detail Instructions
----------------------------------------------------------------

	Please follow the instructions given in DOCS/oaisim_walkthrough.pdf


----------------------------------------------------------------
5) OAI on virtual machine  
----------------------------------------------------------------
	you may download the image using this URL: https://emu.openairinterface.org/openairlab/openairlab.zip
	check that all the packages are there as some packages might be added later.


----------------------------------------------------------------
6) Installing phpmyadmin for HSS  
----------------------------------------------------------------

Step 1:

Configure database for phpmyadmin with dbconfig-common?  SELECT <Yes>


Step 2:

Please provide the password for the administrative account with which this package should create its MySQL database and user.
Password of the database's administrative user:  The mysql administrative user.


Step 3:

Please provide a password for phpmyadmin to register with the database server.  If left blank, a random password will be generated.
MySQL application password for phpmyadmin: admin


Step 4:

Password confirmation: admin


Step 5:

Please choose the web server that should be automatically configured to run phpMyAdmin.
Web server to reconfigure automatically:
        [*] apache2 
        [ ] lighttpd 
