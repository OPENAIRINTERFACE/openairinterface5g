#!/bin/bash
################################################################################
#   OpenAirInterface
#   Copyright(c) 1999 - 2014 Eurecom
#
#    OpenAirInterface is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#
#    OpenAirInterface is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with OpenAirInterface.The full GNU General Public License is
#    included in this distribution in the file called "COPYING". If not,
#    see <http://www.gnu.org/licenses/>.
#
#  Contact Information
#  OpenAirInterface Admin: openair_admin@eurecom.fr
#  OpenAirInterface Tech : openair_tech@eurecom.fr
#  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr
#
#  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.
#
################################################################################
# Tested on ubuntu 12.04 with updates 07 november 2013

$1 rm -Rf /usr/local/src/asn1c-r1516
$1 svn co https://github.com/vlm/asn1c/trunk  /usr/local/src/asn1c-r1516 -r 1516  > /tmp/install_log.txt
cd /usr/local/src/asn1c-r1516
$1 patch -p0 < $OPENAIR3_DIR/S1AP/MESSAGES/ASN1/asn1cpatch.p0  > /tmp/install_log.txt
$1 ./configure  > /tmp/install_log.txt
$1 make  > /tmp/install_log.txt
$1 make install  > /tmp/install_log.txt