#!/bin/bash
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
################################################################################
# Tested on ubuntu 12.04 with updates 07 november 2013

$1 rm -Rf /usr/local/src/asn1c-r1516
$1 svn co https://github.com/vlm/asn1c/trunk  /usr/local/src/asn1c-r1516 -r 1516  > /tmp/install_log.txt
cd /usr/local/src/asn1c-r1516
$1 patch -p0 < $OPENAIR3_DIR/S1AP/MESSAGES/ASN1/asn1cpatch.p0  > /tmp/install_log.txt
$1 ./configure  > /tmp/install_log.txt
$1 make  > /tmp/install_log.txt
$1 make install  > /tmp/install_log.txt
