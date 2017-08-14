This directory contains the necessary scripts and Makefiles to generate all the LTE configuration data structures based on the ASN.1 RRC
source code from the 36.331 RRC specifications.  These structures are used by MAC and PHY for configuration purposes.

It contains the following files

README.txt                                              : This file
Makefile.inc                                            : Makefile to be included by OpenAir compilation scripts for LTE Data structures
asn1c                                                   : Directory to place asn1c package
asn1c/ASN1_files/36331-860.txt                          : Text file containing 36.331-860 specifications used to generate ASN1 source 
asn1c/ASN1_files/extract_asn1_from_spec.pl              : Pearl script to extract ASN.1 source from 36311-860.txt
asn1c/ASN1_files/EUTRA-RRC-Definitions.asn              : First ASN.1 source (generated from above .txt file)
asn1c/ASN1_files/EUTRA-InterNodeDefinitions.asn .asn    : Second ASN.1 source (generated from above .txt file)
asn1c/ASN1_files/EUTRA-UE-Variables.asn                 : Third ASN.1 source (generated from above .txt file)

Instructions to build data structures from ASN1 sources 

The three files have already been built using the extract_asn1_from_spec.pl Pearl script.  This should be used again if a newer version
of the RRC spec is used to synthesize the data structures and encoding/decoding routines.  To do this:
   1. use Microsoft WORD to generate a text version of the 3GPP 36.331 document
   2. run the script on the text file to generate the three files.

Now download the asn1c utility (version 0.9.22) from 

http://lionet.info/asn1c/blog/

or via svn from sourceforge (version 0.9.22)

For version 0.9.2:
svn co https://asn1c.svn.sourceforge.net/svnroot/asn1c/trunk asn1c
For the latest version:
svn co https://github.com/vlm/asn1c/trunk asn1c

and put it in the asn1c directory (you should have a second asn1c directory now). Compile it and install it locally 
(or make sure your $PATH can find the asn1c executable) according to the instructions given (see INSTALL file)
by the author. We have validated that 0.9.22 works on Linux and Cygwin. To generate the data structures do the following from the 
current directory

asn1c -gen-PER -fcompound-names -fnative-types -fskeletons-copy ./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn

Note this only uses one of the three files from 36.331 (the core of the RRC).  After this step you should have many .c and .h files 
and one new Makefile, the latter of which is not used.

If you want to compile the ASN1 sources as a kernel module you need to apply the patch asn1_patch 

patch -p1 < asn1_patch
