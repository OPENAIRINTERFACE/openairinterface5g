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

#vnf unit test
AM_CPPFLAGS = -I../nfapi/inc -I../nfapi/public_inc  -I../common/public_inc -I../vnf/public_inc $(CFLAGS_CUNIT) -Wall -Werror

export LD_LIBRARY_PATH=/opt/gcc-4.7.3/lib
check_PROGRAMS= nfapi_integration_tests
nfapi_integration_tests_SOURCES = main.cpp ../common/src/debug.c   
nfapi_integration_tests_LDADD= -L$(libdir) -lpthread -lrt  -lcunit



LOG_DRIVER = $(top_srcdir)/tap-driver.sh
TESTS=nfapi_integration_tests
EXTRA_DIST=$(TESTS)
