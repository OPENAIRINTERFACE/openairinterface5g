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

#nfapi unit test
AUTOMAKE_OPTIONS=subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/nfapi/inc -I$(top_srcdir)/nfapi/public_inc  -I$(top_srcdir)/common/public_inc -I$(top_srcdir)pnf/public_inc $(CFLAGS_CUNIT) -Wall -Werror
AM_CFLAGS = -I$(top_srcdir)/nfapi/inc -I$(top_srcdir)/nfapi/public_inc  -I$(top_srcdir)/common/public_inc -I$(top_srcdir)/pnf/public_inc $(CFLAGS_CUNIT) -Wall -Werror

check_PROGRAMS= test_nfapi
test_nfapi_SOURCES = nfapi_cunit_main.c ../src/nfapi.c ../src/nfapi_p7.c ../../common/src/debug.c ../src/nfapi_p5.c  ../src/nfapi_p4.c 
test_nfapi_LDADD=$(top_builddir)/pnf/libnfapi_pnf.a  -L$(libdir) -lpthread -lrt -lsctp -lz -lcunit


#jTEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh --ingore-exit
#TEST_ENVIRONMENT = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh

#AM_TESTS_ENVIRONMENT = env AM_TAP_AWK='$(AWK)' $(SHELL);
LOG_DRIVER = $(top_srcdir)/tap-driver.sh
#AM_LOG_FLAGS = -- test_nfapi

TESTS=$(check_PROGRAMS)
EXTRA_DIST = $(TESTS)
