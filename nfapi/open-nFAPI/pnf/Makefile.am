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

AM_CPPFLAGS = -I$(top_srcdir)/pnf/inc -I$(top_srcdir)/pnf/public_inc -I$(top_srcdir)/nfapi/public_inc -I$(top_srcdir)/common/public_inc -g -Wall -Werror


noinst_LIBRARIES = libnfapi_pnf.a

libnfapi_pnf_a_SOURCES = src/pnf.c\
						 src/pnf_interface.c\
						 src/pnf_p7.c\
						 src/pnf_p7_interface.c

LDADD = -lsctp



