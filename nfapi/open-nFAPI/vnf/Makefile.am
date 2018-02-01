#
# Copyright 2017 Cisco Systems, Inc.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 


AUTOMAKE_OPTIONS=subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/vnf/inc\
			  -I$(top_srcdir)/vnf/public_inc\
			  -I$(top_srcdir)/nfapi/public_inc\
			  -I$(top_srcdir)/common/public_inc\
			  -Wall -Werror -g


noinst_LIBRARIES = libnfapi_vnf.a

libnfapi_vnf_a_SOURCES = src/vnf.c\
						 src/vnf_p7.c\
						 src/vnf_interface.c\
						 src/vnf_p7_interface.c

LDADD = -lsctp  ../common/bin/libnfapi_common.a

libnfapi_vnf_a_CFLAGS =$(AM_CFLAGS)

lib_LTLIBRARIES = libnfapi_vnf.la

libnfapi_vnf_la_SOURCES =  src/vnf.c\
						 src/vnf_p7.c\
						 src/vnf_interface.c\
						 src/vnf_p7_interface.c
