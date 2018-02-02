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

AM_CFLAGS = @ADD_CFLAGS@ \
	@CHECK_CFLAGS@	\
	-I$(top_srcdir)/COMMON	\
	-I$(top_srcdir)/GTPV1-U			\
	-I$(top_srcdir)/GTPV1-U/nw-gtpv1u/include	\
	-I$(top_srcdir)/GTPV1-U/nw-gtpv1u/shared	\
	-I$(top_srcdir)/SCTP			\
	-I$(top_srcdir)/INTERTASK_INTERFACE	\
	-I$(top_srcdir)/IPV4			\
	-I$(top_builddir)/S1AP/MESSAGES	\
	-I$(top_builddir)/S1AP			\
	-I$(top_srcdir)/S1AP			\
	-I$(top_srcdir)/SECU			\
	-I$(top_srcdir)/S6A			\
	-I$(top_srcdir)/SGW-LITE		\
	-I$(top_srcdir)/UDP			\
	-I$(top_srcdir)/UTILS			\
	-I$(top_srcdir)/UTILS/TIMER		\
	-I$(top_srcdir)/UTILS/HASTABLE

if HAVE_CHECK
TESTS =	\
	test_kdf	\
	test_aes128_cmac_encrypt	\
	test_aes128_ctr_encrypt	\
	test_aes128_ctr_decrypt	\
	test_secu_kenb	\
	test_secu_knas	\
    test_secu_knas_encrypt_eea1 \
    test_secu_knas_encrypt_eia1 \
    test_secu_knas_encrypt_eea2 \
    test_secu_knas_encrypt_eia2
else
TESTS =
endif

libtest_util_la_SOURCES = \
	test_util.c	test_util.h

noinst_LTLIBRARIES = libtest_util.la 

common_ldadd = 	$(top_builddir)/SECU/libsecu.la	\
	libtest_util.la	\
	$(top_builddir)/UTILS/libutils.la	\
	@CHECK_LIBS@

check_PROGRAMS =	\
	test_kdf	\
	test_aes128_cmac_encrypt	\
	test_aes128_ctr_encrypt	\
	test_aes128_ctr_decrypt	\
	test_secu_kenb	\
	test_secu_knas	\
    test_secu_knas_encrypt_eea1 \
    test_secu_knas_encrypt_eia1 \
    test_secu_knas_encrypt_eea2 \
    test_secu_knas_encrypt_eia2

test_kdf_LDADD =	\
	$(common_ldadd)

test_secu_knas_LDADD =	\
	$(common_ldadd)

test_secu_kenb_LDADD =	\
	$(common_ldadd)

test_secu_knas_encrypt_eea1_LDADD = \
    $(common_ldadd)

test_secu_knas_encrypt_eia1_LDADD = \
    $(common_ldadd)

test_secu_knas_encrypt_eea2_LDADD = \
    $(common_ldadd)

test_secu_knas_encrypt_eia2_LDADD = \
    $(common_ldadd)

test_aes128_ctr_encrypt_LDADD =	\
	$(common_ldadd)

test_aes128_ctr_decrypt_LDADD =	\
	$(common_ldadd)

test_aes128_cmac_encrypt_LDADD =	\
	$(common_ldadd)

test_s1ap_LDADD = libtest_util.la	\
 	$(top_builddir)/S1AP/libs1ap.la
# 
 test_s1ap_SOURCES = test_s1ap.c	\
 	$(top_srcdir)/S1AP/s1ap_eNB_decoder.c \
 	$(top_srcdir)/S1AP/s1ap_eNB_decoder.h \
 	$(top_srcdir)/S1AP/s1ap_eNB_encoder.c \
 	$(top_srcdir)/S1AP/s1ap_eNB_encoder.h
 
 test_secu_kenb_SOURCES = test_secu_kenb.c
 test_secu_knas_SOURCES = test_secu_knas.c
