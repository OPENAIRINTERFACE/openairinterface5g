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

AM_CFLAGS = @ADD_CFLAGS@ 	\
	-I$(top_srcdir)/COMMON	\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/api/network		\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/include			\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/ies				\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/emm/msg			\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/esm/msg			\
	-I$(top_srcdir)/NAS/EURECOM-NAS/src/util			\
	-I$(top_srcdir)/INTERTASK_INTERFACE	\
	-I$(top_srcdir)/GTPV1-U/nw-gtpv1u/include	\
	-I$(top_srcdir)/GTPV1-U/nw-gtpv1u/shared	\
    -I$(top_srcdir)/UTILS   \
    -I$(top_srcdir)/UTILS/HASHTABLE   \
	-I$(top_srcdir)/UTILS/TIMER

noinst_LTLIBRARIES = libgtpv1u.la

libgtpv1u_la_LDFLAGS = -all-static

libgtpv1u_la_SOURCES = \
	gtpv1u_task.c	gtpv1u.h	\
	gtpv1u_teid_pool.c	\
	nw-gtpv1u/shared/NwTypes.h	\
	nw-gtpv1u/shared/NwUtils.h	\
	nw-gtpv1u/shared/NwGtpv1uError.h	\
	nw-gtpv1u/shared/NwLog.h		\
	nw-gtpv1u/shared/NwGtpv1uIe.h	\
	nw-gtpv1u/shared/NwGtpv1uMsg.h	\
	nw-gtpv1u/shared/NwGtpv1u.h	\
	nw-gtpv1u/include/NwGtpv1uPrivate.h	\
	nw-gtpv1u/include/NwGtpv1uLog.h	\
	nw-gtpv1u/include/NwGtpv1uTrxn.h	\
	nw-gtpv1u/include/NwGtpv1uTunnelEndPoint.h	\
	nw-gtpv1u/src/NwGtpv1uTunnelEndPoint.c	\
	nw-gtpv1u/src/NwGtpv1uTrxn.c	\
	nw-gtpv1u/src/NwGtpv1uMsg.c	\
	nw-gtpv1u/src/NwGtpv1u.c
