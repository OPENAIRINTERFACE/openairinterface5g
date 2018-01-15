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
AM_CPPFLAGS = -I$(top_srcdir)/pnf_sim/inc -I$(top_srcdir)/sim_common/inc -I$(top_srcdir)/pnf/public_inc  -I$(top_srcdir)/common/inc -I$(top_srcdir)/common/public_inc -I$(top_srcdir)/nfapi/public_inc -I$(top_srcdir)/nfapi/inc $(XML_CFLAGS) -g
AM_CXXFLAGS = -I$(top_srcdir)/pnf_sim/inc -I$(top_srcdir)/sim_common/inc -I$(top_srcdir)/pnf/public_inc  -I$(top_srcdir)/common/inc -I$(top_srcdir)/common/public_inc -I$(top_srcdir)/nfapi/public_inc -I$(top_srcdir)/nfapi/inc $(XML_CFLAGS) -std=c++11 $(BOOST_CPPFLAGS) -g
bin_PROGRAMS = pnfsim
pnfsim_SOURCES = src/main.cpp src/fapi_stub.cpp
LDADD= $(top_builddir)/pnf/libnfapi_pnf.a $(top_builddir)/common/libnfapi_common.a $(top_builddir)/nfapi/libnfapi.a $(top_builddir)/sim_common/libnfapi_sim_common.a -L$(libdir) -lpthread -lrt -lsctp  -lz 
