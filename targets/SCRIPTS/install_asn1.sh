#!/bin/bash
which asn1c > /dev/null
if [ $? -eq 0 ] ; then echo "asn1c is installed" ; else echo "Please install asn1c (version 0.9.22 or greater)" ; fi
cd $OPENAIR2_DIR/RRC/LITE/MESSAGES && asn1c -gen-PER -fcompound-names -fnative-types -fskeletons-copy $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/ASN1_files/EUTRA-RRC-Definitions.asn


