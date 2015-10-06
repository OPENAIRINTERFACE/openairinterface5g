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