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

AM_CFLAGS = -I$(top_srcdir)/sim_common/public_inc -I$(top_srcdir)/common/public_inc -I$(top_srcdir)/nfapi/public_inc -g -Wall -Werror

AM_CXXFLAGS = -I$(top_srcdir)/sim_common/public_inc -I$(top_srcdir)/common/public_inc -std=c++11 -I$(top_srcdir)/nfapi/public_inc $(BOOST_CPPFLAGS) -g -Wall -Werror
noinst_LIBRARIES =libnfapi_sim_common.a
#libnfapi_sim_common_a_SOURCES = src/xml_property_tree.cpp src/pool.cpp
libnfapi_sim_common_a_SOURCES = src/pool.cpp

libnfapi_sim_common_a_CFLAGS =$(AM_CFLAGS)
libnfapi_sim_common_a_CXXFLAGS =$(AM_CXXFLAGS)

lib_LTLIBRARIES =libnfapi_sim_common.la
#libnfapi_sim_common_la_SOURCES = src/xml_property_tree.cpp
libnfapi_sim_common_la_SOURCES = src/pool.cpp
