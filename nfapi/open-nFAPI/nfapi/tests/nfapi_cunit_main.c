/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CUnit.h"
#include "Basic.h"
#include "Automated.h"
//#include "CUnit/Console.h"

#include "nfapi_interface.h"
#include "nfapi.h"
#include <stdio.h>  // for printf
#include <stdlib.h>
#include "debug.h"
#include "nfapi_interface.h"
/* Test Suite setup and cleanup functions: */

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

#define MAX_PACKED_MESSAGE_SIZE	8192

#define IN_OUT_ASSERT(_V) { CU_ASSERT_EQUAL(in._V, out._V); }

typedef struct 
{
	nfapi_tl_t tl;
	uint16_t value1;
	uint32_t value2;
} my_vendor_extention;

extern void* nfapi_allocate_pdu(size_t size)
{
	return malloc(size);
}

extern int nfapi_test_unpack_vendor_extension_tlv(nfapi_tl_t* tl, uint8_t **ppReadPackedMsg, uint8_t *end, void** ve, nfapi_p4_p5_codec_config_t* config)
{
	printf("nfapi_unpack_vendor_extension_tlv\n");

	if(tl->tag == NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE)
	{
		my_vendor_extention* mve = (my_vendor_extention*)malloc(sizeof(my_vendor_extention));
		mve->tl.tag = NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE;
		
		if(!(pull16(ppReadPackedMsg, &mve->value1, end) &&
		     pull32(ppReadPackedMsg, &mve->value2, end)))
		{
			free(mve);
			return 0;
		}

		(*ve) = mve;

	}

	return 1;
}

int nfapi_pack_vendor_extension_tlv_called = 0;
extern int nfapi_test_pack_vendor_extension_tlv(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config)
{
	printf("nfapi_pack_vendor_extension_tlv\n");
	nfapi_pack_vendor_extension_tlv_called++;

	my_vendor_extention* mve = (my_vendor_extention*)ve;
	
	return (push16(mve->value1, ppWritePackedMsg, end) &&
			push32(mve->value2, ppWritePackedMsg, end));

}

uint8_t gTestNfapiMessageTx[MAX_PACKED_MESSAGE_SIZE];


uint8_t rand8(uint8_t min, uint8_t max)
{
	return ((rand() % (max + 1 - min)) + min);
}

/************* Test case functions ****************/

void test_case_sample(void)
{
	/*Sample assert function, use as per your test case scenario.*/
	CU_ASSERT(CU_TRUE);
	CU_ASSERT_NOT_EQUAL(2, -1);
	CU_ASSERT_STRING_EQUAL("string #1", "string #1");
	CU_ASSERT_STRING_NOT_EQUAL("string #1", "string #2");

	CU_ASSERT(CU_FALSE);
	CU_ASSERT_EQUAL(2, 3);
	CU_ASSERT_STRING_NOT_EQUAL("string #1", "string #1");
	CU_ASSERT_STRING_EQUAL("string #1", "string #2");
}


int nfapi_test_verify_configreq_params(nfapi_pnf_config_request_t pnfConfigRequest_in, nfapi_pnf_config_request_t pnfConfigRequest_out)
{
	int i=0;
	if(memcmp(&pnfConfigRequest_in, &pnfConfigRequest_out, sizeof(nfapi_pnf_config_request_t)) == 0)
	{	
		/*packed and unpacked phy config request are matched.*/

		printf("Success: packed and unpacked phy config request are matched\n");
		return 1;
	}
	else
	{
		if (pnfConfigRequest_in.header.message_id != pnfConfigRequest_out.header.message_id)
		{
			printf("Mismatch: packed message id %d, unpacked message id %d\n",
					pnfConfigRequest_in.header.message_id, pnfConfigRequest_out.header.message_id);
		}
		if (pnfConfigRequest_in.header.message_length != pnfConfigRequest_out.header.message_length)
		{
			printf("Mismatch: packed message_length %d, unpacked message_length %d\n",
					pnfConfigRequest_in.header.message_length, pnfConfigRequest_out.header.message_length);
		}
		if (pnfConfigRequest_in.header.phy_id != pnfConfigRequest_out.header.phy_id)
		{
			printf("Mismatch: packed phy_id %d, unpacked phy_id %d\n",
					pnfConfigRequest_in.header.phy_id, pnfConfigRequest_out.header.phy_id);
		}
		if (pnfConfigRequest_in.pnf_phy_rf_config.number_phy_rf_config_info != pnfConfigRequest_out.pnf_phy_rf_config.number_phy_rf_config_info)			
		{
			printf("Mismatch: packed number_phy_rf_config_info %d, unpacked number_phy_rf_config_info %d\n",
					pnfConfigRequest_in.pnf_phy_rf_config.number_phy_rf_config_info, pnfConfigRequest_out.pnf_phy_rf_config.number_phy_rf_config_info);
		}

		for (i=0; i < NFAPI_MAX_PHY_RF_INSTANCES; i++) 
		{
			if (pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].phy_id != pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].phy_id)			
			{
				printf("Mismatch: rf idx %d, packed phy id %d, unpacked phy id %d\n",
						i, pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].phy_id, pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].phy_id);
			}
			if (pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].phy_config_index!= pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].phy_config_index)			
			{
				printf("Mismatch: rf idx %d, packed phy_config_index %d, unpacked phy_config_index %d\n",
						i, pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].phy_config_index, pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].phy_config_index);
			}
			if (pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].rf_config_index != pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].rf_config_index)			
			{
				printf("Mismatch: rf idx %d, packed rf_config_index %d, unpacked rf_config_index %d\n",
						i, pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[i].rf_config_index, pnfConfigRequest_out.pnf_phy_rf_config.phy_rf_config[i].rf_config_index);
			}
		}
		return 0;
	}

}


void nfapi_test_pnf_config_req(void) 
{

	nfapi_pnf_config_request_t pnfConfigRequest_out;
	nfapi_pnf_config_request_t pnfConfigRequest_in;
	int packedMessageLength =0;

	printf(" nfapi_test_pnf_config_req run \n");

	/* Build phy config req */
	pnfConfigRequest_in.header.phy_id = NFAPI_PHY_ID_NA;
	pnfConfigRequest_in.header.message_id = NFAPI_PNF_CONFIG_REQUEST;
	pnfConfigRequest_in.header.message_length = 22;
	pnfConfigRequest_in.header.spare = 0;

	pnfConfigRequest_in.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
	pnfConfigRequest_in.pnf_phy_rf_config.tl.length = 0;
	pnfConfigRequest_in.pnf_phy_rf_config.number_phy_rf_config_info = 2;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[0].phy_id = 4;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[0].phy_config_index = 1;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[0].rf_config_index = 1;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[1].phy_id = 5;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[1].phy_config_index = 2;
	pnfConfigRequest_in.pnf_phy_rf_config.phy_rf_config[1].rf_config_index = 2;

	/*Pack pnf config req*/
	packedMessageLength = nfapi_p5_message_pack(&pnfConfigRequest_in, sizeof(nfapi_pnf_config_request_t), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	/*Unpack pnf config request*/
	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &pnfConfigRequest_out, sizeof(nfapi_pnf_config_request_t), 0);

	/*Verify unpacked message*/
	CU_ASSERT_EQUAL(nfapi_test_verify_configreq_params(pnfConfigRequest_in, pnfConfigRequest_out), 1);  

}

void nfapi_test_2(void) {
	/*Sample test function*/
	CU_ASSERT_EQUAL( 0, 0);
}

void nfapi_test_rssi_request_lte()
{
	uint16_t idx;
	nfapi_rssi_request_t in;
	nfapi_rssi_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.lte_rssi_request.tl.tag = NFAPI_LTE_RSSI_REQUEST_TAG;
	in.lte_rssi_request.frequency_band_indicator = 0;
	in.lte_rssi_request.bandwidth = 0;
	in.lte_rssi_request.timeout = 0;
	in.lte_rssi_request.number_of_earfcns = 4;
	in.lte_rssi_request.earfcn[0] = 42;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int upack_result = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_NOT_EQUAL(packedMessageLength, -1);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, 0);
	CU_ASSERT_NOT_EQUAL(upack_result, 0);
	CU_ASSERT_NOT_EQUAL(upack_result, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_rssi_request.frequency_band_indicator, out.lte_rssi_request.frequency_band_indicator);
	CU_ASSERT_EQUAL(in.lte_rssi_request.bandwidth, out.lte_rssi_request.bandwidth);
	CU_ASSERT_EQUAL(in.lte_rssi_request.timeout, out.lte_rssi_request.timeout);
	CU_ASSERT_EQUAL(in.lte_rssi_request.number_of_earfcns, out.lte_rssi_request.number_of_earfcns);

	for(idx = 0; idx < out.lte_rssi_request.number_of_earfcns; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_rssi_request.earfcn[idx], out.lte_rssi_request.earfcn[idx]);
	}
}

void nfapi_test_rssi_request_lte2()
{
	uint16_t idx;
	nfapi_rssi_request_t in;
	nfapi_rssi_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.lte_rssi_request.tl.tag = NFAPI_LTE_RSSI_REQUEST_TAG;
	in.lte_rssi_request.frequency_band_indicator = 0;
	in.lte_rssi_request.bandwidth = 0;
	in.lte_rssi_request.timeout = 0;
	in.lte_rssi_request.number_of_earfcns = 4;
	in.lte_rssi_request.earfcn[0] = 42;

	// try and pak with a buffer that is too small
	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, NFAPI_HEADER_LENGTH + 4, 0);
	CU_ASSERT_EQUAL(packedMessageLength, -1);

	// pack correctly
	packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	// unpack with a small bufer
	int upack_result = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength - 1, &out, sizeof(out), 0);

	upack_result = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_NOT_EQUAL(packedMessageLength, -1);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, 0);
	CU_ASSERT_NOT_EQUAL(upack_result, 0);
	CU_ASSERT_NOT_EQUAL(upack_result, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_rssi_request.frequency_band_indicator, out.lte_rssi_request.frequency_band_indicator);
	CU_ASSERT_EQUAL(in.lte_rssi_request.bandwidth, out.lte_rssi_request.bandwidth);
	CU_ASSERT_EQUAL(in.lte_rssi_request.timeout, out.lte_rssi_request.timeout);
	CU_ASSERT_EQUAL(in.lte_rssi_request.number_of_earfcns, out.lte_rssi_request.number_of_earfcns);

	for(idx = 0; idx < out.lte_rssi_request.number_of_earfcns; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_rssi_request.earfcn[idx], out.lte_rssi_request.earfcn[idx]);
	}
}



// Test decoding of message which has a large number if earfcn which could
// cause a buffer over run if not handled
void nfapi_test_rssi_request_lte_overrun()
{
	nfapi_rssi_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t* end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_RSSI_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_LTE, &p, end);
	push16(NFAPI_LTE_RSSI_REQUEST_TAG, &p, end);
	push16(11, &p, end);

	push8(0, &p, end);
	push16(0, &p, end);
	push8(0, &p, end);
	push32(0, &p, end);
	push8(123, &p, end); // to many EARFCN's
	push16(16, &p, end);

	int result = nfapi_p4_message_unpack(buffer, sizeof(buffer), &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_rssi_request_lte_rat_type_mismatch()
{
	nfapi_rssi_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_RSSI_REQUEST, &p, end);
	push16(16, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_GERAN, &p, end);

	push16(NFAPI_LTE_RSSI_REQUEST_TAG, &p, end);
	push16(11, &p, end);

	push8(0, &p, end);
	push16(0, &p, end);
	push8(0, &p, end);
	push32(0, &p, end);
	push8(1, &p, end);
	push16(16, &p, end);

	int result = nfapi_p4_message_unpack(buffer, sizeof(buffer), &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_rssi_request_utran()
{
	uint16_t idx;
	nfapi_rssi_request_t in;
	nfapi_rssi_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_UTRAN;


	in.utran_rssi_request.tl.tag = NFAPI_UTRAN_RSSI_REQUEST_TAG;
	in.utran_rssi_request.frequency_band_indicator = 0;
	in.utran_rssi_request.measurement_period = 0;
	in.utran_rssi_request.timeout = 0;
	in.utran_rssi_request.number_of_uarfcns = 3;
	in.utran_rssi_request.uarfcn[0] = 42;
	in.utran_rssi_request.uarfcn[1] = 42;
	in.utran_rssi_request.uarfcn[2] = 42;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.utran_rssi_request.frequency_band_indicator, out.utran_rssi_request.frequency_band_indicator);
	CU_ASSERT_EQUAL(in.utran_rssi_request.measurement_period, out.utran_rssi_request.measurement_period);
	CU_ASSERT_EQUAL(in.utran_rssi_request.timeout, out.utran_rssi_request.timeout);
	CU_ASSERT_EQUAL(in.utran_rssi_request.number_of_uarfcns, out.utran_rssi_request.number_of_uarfcns);

	for(idx = 0; idx < out.utran_rssi_request.number_of_uarfcns; ++idx)
	{
		CU_ASSERT_EQUAL(in.utran_rssi_request.uarfcn[idx], out.utran_rssi_request.uarfcn[idx]);
	}

}

void nfapi_test_rssi_request_geran()
{
	uint16_t idx;
	nfapi_rssi_request_t in;
	nfapi_rssi_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_GERAN;

	in.geran_rssi_request.tl.tag = NFAPI_GERAN_RSSI_REQUEST_TAG;
	in.geran_rssi_request.frequency_band_indicator = 0;
	in.geran_rssi_request.measurement_period = 0;
	in.geran_rssi_request.timeout = 0;
	in.geran_rssi_request.number_of_arfcns = 3;
	in.geran_rssi_request.arfcn[0].arfcn = 42;
	in.geran_rssi_request.arfcn[0].direction = 0;
	in.geran_rssi_request.arfcn[1].arfcn = 42;
	in.geran_rssi_request.arfcn[1].direction = 1;
	in.geran_rssi_request.arfcn[2].arfcn = 42;
	in.geran_rssi_request.arfcn[2].direction = 1;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.geran_rssi_request.frequency_band_indicator, out.geran_rssi_request.frequency_band_indicator);
	CU_ASSERT_EQUAL(in.geran_rssi_request.measurement_period, out.geran_rssi_request.measurement_period);
	CU_ASSERT_EQUAL(in.geran_rssi_request.timeout, out.geran_rssi_request.timeout);
	CU_ASSERT_EQUAL(in.geran_rssi_request.number_of_arfcns, out.geran_rssi_request.number_of_arfcns);

	for(idx = 0; idx < out.geran_rssi_request.number_of_arfcns; ++idx)
	{
		CU_ASSERT_EQUAL(in.geran_rssi_request.arfcn[idx].arfcn, out.geran_rssi_request.arfcn[idx].arfcn);
		CU_ASSERT_EQUAL(in.geran_rssi_request.arfcn[idx].direction, out.geran_rssi_request.arfcn[idx].direction);
	}
}

void nfapi_test_rssi_request_nb_iot()
{
	uint16_t idx, idx2;
	nfapi_rssi_request_t in;
	nfapi_rssi_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_NB_IOT;

	in.nb_iot_rssi_request.tl.tag = NFAPI_NB_IOT_RSSI_REQUEST_TAG;
	in.nb_iot_rssi_request.frequency_band_indicator = 34;
	in.nb_iot_rssi_request.measurement_period = 900;
	in.nb_iot_rssi_request.timeout = 4321;
	in.nb_iot_rssi_request.number_of_earfcns = 3;
	in.nb_iot_rssi_request.earfcn[0].earfcn = 42;
	in.nb_iot_rssi_request.earfcn[0].number_of_ro_dl = 1;
	in.nb_iot_rssi_request.earfcn[0].ro_dl[0] = 2;
	in.nb_iot_rssi_request.earfcn[1].earfcn = 43;
	in.nb_iot_rssi_request.earfcn[1].number_of_ro_dl = 0;
	in.nb_iot_rssi_request.earfcn[2].earfcn = 44;
	in.nb_iot_rssi_request.earfcn[2].number_of_ro_dl = 2;
	in.nb_iot_rssi_request.earfcn[2].ro_dl[0] = 0;
	in.nb_iot_rssi_request.earfcn[2].ro_dl[1] = 4;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	IN_OUT_ASSERT(header.message_id);
	IN_OUT_ASSERT(rat_type);
	IN_OUT_ASSERT(nb_iot_rssi_request.frequency_band_indicator);
	IN_OUT_ASSERT(nb_iot_rssi_request.measurement_period);
	IN_OUT_ASSERT(nb_iot_rssi_request.timeout);
	IN_OUT_ASSERT(nb_iot_rssi_request.number_of_earfcns);

	for(idx = 0; idx < out.nb_iot_rssi_request.number_of_earfcns; ++idx)
	{
		IN_OUT_ASSERT(nb_iot_rssi_request.earfcn[idx].earfcn);
		IN_OUT_ASSERT(nb_iot_rssi_request.earfcn[idx].number_of_ro_dl);
		
		for(idx2 = 0; idx2 < out.nb_iot_rssi_request.earfcn[idx].number_of_ro_dl; ++idx2)
			IN_OUT_ASSERT(nb_iot_rssi_request.earfcn[idx].ro_dl[idx2]);
	}
}


void nfapi_test_rssi_response()
{
	nfapi_rssi_response_t in;
	nfapi_rssi_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);

}

void nfapi_test_rssi_indication()
{
	uint16_t idx = 0;
	nfapi_rssi_indication_t in;
	nfapi_rssi_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RSSI_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_OK;
	in.rssi_indication_body.tl.tag = NFAPI_RSSI_INDICATION_TAG;
	in.rssi_indication_body.tl.length = 0;
	in.rssi_indication_body.number_of_rssi = 4;
	in.rssi_indication_body.rssi[0] = 2;
	in.rssi_indication_body.rssi[1] = 0;
	in.rssi_indication_body.rssi[2] = -23;
	in.rssi_indication_body.rssi[3] = -18000;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.rssi_indication_body.tl.tag, out.rssi_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.rssi_indication_body.number_of_rssi, out.rssi_indication_body.number_of_rssi);

	for(idx = 0; idx < out.rssi_indication_body.number_of_rssi; ++idx)
	{
		CU_ASSERT_EQUAL(in.rssi_indication_body.rssi[idx], out.rssi_indication_body.rssi[idx]);
	}

}
void nfapi_test_rssi_indication_overrun()
{
	nfapi_rssi_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_RSSI_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error code

	push16(NFAPI_RSSI_INDICATION_TAG, &p, end);
	push16(11, &p, end);

	push16(NFAPI_MAX_RSSI + 23, &p, end); // to many EARFCN's
	push16(16, &p, end);

	int result = nfapi_p4_message_unpack(buffer, sizeof(buffer), &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_cell_search_request_lte()
{
	uint16_t idx;
	nfapi_cell_search_request_t in;
	nfapi_cell_search_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.lte_cell_search_request.tl.tag = NFAPI_LTE_CELL_SEARCH_REQUEST_TAG;
	in.lte_cell_search_request.earfcn = 213;
	in.lte_cell_search_request.measurement_bandwidth = 1;
	in.lte_cell_search_request.exhaustive_search = 0;
	in.lte_cell_search_request.timeout = 123;
	in.lte_cell_search_request.number_of_pci = 3;
	in.lte_cell_search_request.pci[0] = 3;
	in.lte_cell_search_request.pci[1] = 6;
	in.lte_cell_search_request.pci[2] = 9;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_cell_search_request.earfcn, out.lte_cell_search_request.earfcn);
	CU_ASSERT_EQUAL(in.lte_cell_search_request.measurement_bandwidth, out.lte_cell_search_request.measurement_bandwidth);
	CU_ASSERT_EQUAL(in.lte_cell_search_request.exhaustive_search, out.lte_cell_search_request.exhaustive_search);
	CU_ASSERT_EQUAL(in.lte_cell_search_request.timeout, out.lte_cell_search_request.timeout);
	CU_ASSERT_EQUAL(in.lte_cell_search_request.number_of_pci, out.lte_cell_search_request.number_of_pci);

	for(idx = 0; idx < out.lte_cell_search_request.number_of_pci; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_cell_search_request.pci[idx], out.lte_cell_search_request.pci[idx]);
	}
}

void nfapi_test_cell_search_request_utran()
{
	uint16_t idx;
	nfapi_cell_search_request_t in;
	nfapi_cell_search_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_UTRAN;

	in.utran_cell_search_request.tl.tag = NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG;
	in.utran_cell_search_request.uarfcn = 213;
	in.utran_cell_search_request.exhaustive_search = 0;
	in.utran_cell_search_request.timeout = 123;
	in.utran_cell_search_request.number_of_psc = 3;
	in.utran_cell_search_request.psc[0] = 3;
	in.utran_cell_search_request.psc[1] = 6;
	in.utran_cell_search_request.psc[2] = 9;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.utran_cell_search_request.uarfcn, out.utran_cell_search_request.uarfcn);
	CU_ASSERT_EQUAL(in.utran_cell_search_request.exhaustive_search, out.utran_cell_search_request.exhaustive_search);
	CU_ASSERT_EQUAL(in.utran_cell_search_request.timeout, out.utran_cell_search_request.timeout);
	CU_ASSERT_EQUAL(in.utran_cell_search_request.number_of_psc, out.utran_cell_search_request.number_of_psc);

	for(idx = 0; idx < out.utran_cell_search_request.number_of_psc; ++idx)
	{
		CU_ASSERT_EQUAL(in.utran_cell_search_request.psc[idx], out.utran_cell_search_request.psc[idx]);
	}
}

void nfapi_test_cell_search_request_geran()
{
	uint16_t idx;
	nfapi_cell_search_request_t in;
	nfapi_cell_search_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_GERAN;

	in.geran_cell_search_request.tl.tag = NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG;
	in.geran_cell_search_request.timeout = 123;
	in.geran_cell_search_request.number_of_arfcn = 3;
	in.geran_cell_search_request.arfcn[0] = 3;
	in.geran_cell_search_request.arfcn[1] = 6;
	in.geran_cell_search_request.arfcn[2] = 9;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.geran_cell_search_request.timeout, out.geran_cell_search_request.timeout);
	CU_ASSERT_EQUAL(in.geran_cell_search_request.number_of_arfcn, out.geran_cell_search_request.number_of_arfcn);

	for(idx = 0; idx < out.geran_cell_search_request.number_of_arfcn; ++idx)
	{
		CU_ASSERT_EQUAL(in.geran_cell_search_request.arfcn[idx], out.geran_cell_search_request.arfcn[idx]);
	}
}

void nfapi_test_cell_search_request_nb_iot()
{
	uint16_t idx;
	nfapi_cell_search_request_t in;
	nfapi_cell_search_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_NB_IOT;

	in.nb_iot_cell_search_request.tl.tag = NFAPI_NB_IOT_CELL_SEARCH_REQUEST_TAG;
	in.nb_iot_cell_search_request.earfcn = 54;
	in.nb_iot_cell_search_request.ro_dl = 3;
	in.nb_iot_cell_search_request.exhaustive_search = 1;
	in.nb_iot_cell_search_request.timeout = 123;
	in.nb_iot_cell_search_request.number_of_pci = 3;
	in.nb_iot_cell_search_request.pci[0] = 3;
	in.nb_iot_cell_search_request.pci[1] = 6;
	in.nb_iot_cell_search_request.pci[2] = 9;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	IN_OUT_ASSERT(header.message_id);
	IN_OUT_ASSERT(rat_type);
	IN_OUT_ASSERT(nb_iot_cell_search_request.earfcn);
	IN_OUT_ASSERT(nb_iot_cell_search_request.ro_dl);
	IN_OUT_ASSERT(nb_iot_cell_search_request.exhaustive_search);
	IN_OUT_ASSERT(nb_iot_cell_search_request.timeout);
	IN_OUT_ASSERT(nb_iot_cell_search_request.number_of_pci);

	for(idx = 0; idx < out.nb_iot_cell_search_request.number_of_pci; ++idx)
	{
		IN_OUT_ASSERT(nb_iot_cell_search_request.pci[idx]);
	}
}

void nfapi_test_cell_search_response()
{
	nfapi_cell_search_response_t in;
	nfapi_cell_search_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}

void nfapi_test_cell_search_indication_lte()
{
	uint16_t idx = 0;
	nfapi_cell_search_indication_t in;
	nfapi_cell_search_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_OK;
	in.lte_cell_search_indication.tl.tag = NFAPI_LTE_CELL_SEARCH_INDICATION_TAG;
	in.lte_cell_search_indication.number_of_lte_cells_found = 2;
	in.lte_cell_search_indication.lte_found_cells[0].pci = 0;
	in.lte_cell_search_indication.lte_found_cells[0].rsrp = 42;
	in.lte_cell_search_indication.lte_found_cells[0].rsrq= 11;
	in.lte_cell_search_indication.lte_found_cells[0].frequency_offset = -100;

	in.lte_cell_search_indication.lte_found_cells[1].pci = 123;
	in.lte_cell_search_indication.lte_found_cells[1].rsrp = 2;
	in.lte_cell_search_indication.lte_found_cells[1].rsrq= 17;
	in.lte_cell_search_indication.lte_found_cells[1].frequency_offset = 123;

	in.utran_cell_search_indication.tl.tag = 0;
	in.geran_cell_search_indication.tl.tag = 0;
	in.nb_iot_cell_search_indication.tl.tag = 0;
	in.pnf_cell_search_state.tl.tag = 0;


	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_cell_search_indication.tl.tag, out.lte_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.lte_cell_search_indication.number_of_lte_cells_found, out.lte_cell_search_indication.number_of_lte_cells_found);

	for(idx = 0; idx < out.lte_cell_search_indication.number_of_lte_cells_found; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_cell_search_indication.lte_found_cells[idx].pci, out.lte_cell_search_indication.lte_found_cells[idx].pci);
		CU_ASSERT_EQUAL(in.lte_cell_search_indication.lte_found_cells[idx].rsrp, out.lte_cell_search_indication.lte_found_cells[idx].rsrp);
		CU_ASSERT_EQUAL(in.lte_cell_search_indication.lte_found_cells[idx].rsrq, out.lte_cell_search_indication.lte_found_cells[idx].rsrq);
		CU_ASSERT_EQUAL(in.lte_cell_search_indication.lte_found_cells[idx].frequency_offset, out.lte_cell_search_indication.lte_found_cells[idx].frequency_offset);
	}


	CU_ASSERT_EQUAL(in.utran_cell_search_indication.tl.tag, out.utran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_cell_search_indication.tl.tag, out.geran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_search_state.tl.tag, out.pnf_cell_search_state.tl.tag);
}
void nfapi_test_cell_search_indication_utran()
{
	uint16_t idx = 0;
	nfapi_cell_search_indication_t in;
	nfapi_cell_search_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_OK;
	in.utran_cell_search_indication.tl.tag = NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG;
	in.utran_cell_search_indication.number_of_utran_cells_found = 1;
	in.utran_cell_search_indication.utran_found_cells[0].psc = 123;
	in.utran_cell_search_indication.utran_found_cells[0].rscp = 2;
	in.utran_cell_search_indication.utran_found_cells[0].ecno = 89;
	in.utran_cell_search_indication.utran_found_cells[0].frequency_offset = -1000;

	in.lte_cell_search_indication.tl.tag = 0;
	in.geran_cell_search_indication.tl.tag = 0;
	in.nb_iot_cell_search_indication.tl.tag = 0;
	in.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	in.pnf_cell_search_state.length = 12;
	in.pnf_cell_search_state.value[0] = 34;
	in.pnf_cell_search_state.value[1] = 35;
	in.pnf_cell_search_state.value[2] = 36;
	in.pnf_cell_search_state.value[3] = 37;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.utran_cell_search_indication.tl.tag, out.utran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_cell_search_indication.number_of_utran_cells_found, out.utran_cell_search_indication.number_of_utran_cells_found);

	for(idx = 0; idx < out.utran_cell_search_indication.number_of_utran_cells_found; ++idx)
	{
		CU_ASSERT_EQUAL(in.utran_cell_search_indication.utran_found_cells[idx].psc, out.utran_cell_search_indication.utran_found_cells[idx].psc);
		CU_ASSERT_EQUAL(in.utran_cell_search_indication.utran_found_cells[idx].rscp, out.utran_cell_search_indication.utran_found_cells[idx].rscp);
		CU_ASSERT_EQUAL(in.utran_cell_search_indication.utran_found_cells[idx].ecno, out.utran_cell_search_indication.utran_found_cells[idx].ecno);
		CU_ASSERT_EQUAL(in.utran_cell_search_indication.utran_found_cells[idx].frequency_offset, out.utran_cell_search_indication.utran_found_cells[idx].frequency_offset);
	}

	CU_ASSERT_EQUAL(in.lte_cell_search_indication.tl.tag, out.lte_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_cell_search_indication.tl.tag, out.geran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_search_state.tl.tag, out.pnf_cell_search_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_search_state.length, out.pnf_cell_search_state.length);

	for(idx = 0; idx < out.pnf_cell_search_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_search_state.value[idx], out.pnf_cell_search_state.value[idx]);
	}
}
void nfapi_test_cell_search_indication_geran()
{
	uint16_t idx = 0;
	nfapi_cell_search_indication_t in;
	nfapi_cell_search_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_OK;
	in.geran_cell_search_indication.tl.tag = NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG;
	in.geran_cell_search_indication.number_of_gsm_cells_found = 1;
	in.geran_cell_search_indication.gsm_found_cells[0].arfcn = 123;
	in.geran_cell_search_indication.gsm_found_cells[0].bsic = 2;
	in.geran_cell_search_indication.gsm_found_cells[0].rxlev = 89;
	in.geran_cell_search_indication.gsm_found_cells[0].rxqual = 12;
	in.geran_cell_search_indication.gsm_found_cells[0].frequency_offset = 2389;
	in.geran_cell_search_indication.gsm_found_cells[0].sfn_offset = 23;

	in.lte_cell_search_indication.tl.tag = 0;
	in.utran_cell_search_indication.tl.tag = 0;
	in.nb_iot_cell_search_indication.tl.tag = 0;
	in.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	in.pnf_cell_search_state.length = 63;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.geran_cell_search_indication.tl.tag, out.geran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_cell_search_indication.number_of_gsm_cells_found, out.geran_cell_search_indication.number_of_gsm_cells_found);

	for(idx = 0; idx < out.geran_cell_search_indication.number_of_gsm_cells_found; ++idx)
	{
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].arfcn, out.geran_cell_search_indication.gsm_found_cells[idx].arfcn);
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].bsic, out.geran_cell_search_indication.gsm_found_cells[idx].bsic);
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].rxlev, out.geran_cell_search_indication.gsm_found_cells[idx].rxlev);
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].rxqual, out.geran_cell_search_indication.gsm_found_cells[idx].rxqual);
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].frequency_offset, out.geran_cell_search_indication.gsm_found_cells[idx].frequency_offset);
		CU_ASSERT_EQUAL(in.geran_cell_search_indication.gsm_found_cells[idx].sfn_offset, out.geran_cell_search_indication.gsm_found_cells[idx].sfn_offset);
	}

	CU_ASSERT_EQUAL(in.lte_cell_search_indication.tl.tag, out.lte_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_cell_search_indication.tl.tag, out.utran_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_cell_search_indication.tl.tag, out.nb_iot_cell_search_indication.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_search_state.tl.tag, out.pnf_cell_search_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_search_state.length, out.pnf_cell_search_state.length);

	for(idx = 0; idx < out.pnf_cell_search_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_search_state.value[idx], out.pnf_cell_search_state.value[idx]);
	}
}

void nfapi_test_cell_search_indication_nb_iot()
{
	uint16_t idx = 0;
	nfapi_cell_search_indication_t in;
	nfapi_cell_search_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_OK;
	in.nb_iot_cell_search_indication.tl.tag = NFAPI_NB_IOT_CELL_SEARCH_INDICATION_TAG;
	in.nb_iot_cell_search_indication.number_of_nb_iot_cells_found = 1;
	in.nb_iot_cell_search_indication.nb_iot_found_cells[0].pci = 123;
	in.nb_iot_cell_search_indication.nb_iot_found_cells[0].rsrp = 2;
	in.nb_iot_cell_search_indication.nb_iot_found_cells[0].rsrq = 89;
	in.nb_iot_cell_search_indication.nb_iot_found_cells[0].frequency_offset = 2389;

	in.lte_cell_search_indication.tl.tag = 0;
	in.utran_cell_search_indication.tl.tag = 0;
	in.geran_cell_search_indication.tl.tag = 0;
	
	in.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	in.pnf_cell_search_state.length = 63;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	IN_OUT_ASSERT(header.message_id);
	IN_OUT_ASSERT(error_code);
	IN_OUT_ASSERT(nb_iot_cell_search_indication.tl.tag);
	IN_OUT_ASSERT(nb_iot_cell_search_indication.number_of_nb_iot_cells_found);

	for(idx = 0; idx < out.nb_iot_cell_search_indication.number_of_nb_iot_cells_found; ++idx)
	{
		IN_OUT_ASSERT(nb_iot_cell_search_indication.nb_iot_found_cells[idx].pci);
		IN_OUT_ASSERT(nb_iot_cell_search_indication.nb_iot_found_cells[idx].rsrp);
		IN_OUT_ASSERT(nb_iot_cell_search_indication.nb_iot_found_cells[idx].rsrq);
		IN_OUT_ASSERT(nb_iot_cell_search_indication.nb_iot_found_cells[idx].frequency_offset);
	}

	IN_OUT_ASSERT(lte_cell_search_indication.tl.tag);
	IN_OUT_ASSERT(utran_cell_search_indication.tl.tag);
	IN_OUT_ASSERT(geran_cell_search_indication.tl.tag);
	IN_OUT_ASSERT(pnf_cell_search_state.tl.tag);
	IN_OUT_ASSERT(pnf_cell_search_state.length);
	
	for(idx = 0; idx < out.pnf_cell_search_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_search_state.value[idx], out.pnf_cell_search_state.value[idx]);
	}
}

void nfapi_test_cell_search_request_lte_overrun()
{
	nfapi_cell_search_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(0, &p, end); // rat_type

	push16(NFAPI_LTE_CELL_SEARCH_REQUEST_TAG, &p, end);
	push16(8, &p, end);

	push16(11, &p, end);
	push8(11, &p, end);
	push8(11, &p, end);
	push32(11, &p, end);
	push8(NFAPI_MAX_PCI_LIST + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);

}
void nfapi_test_cell_search_request_utran_overrun()
{
	nfapi_cell_search_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_UTRAN, &p, end); // rat_type

	push16(NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG, &p, end);
	push16(8, &p, end);

	push16(11, &p, end);
	push8(11, &p, end);
	push32(11, &p, end);
	push8(NFAPI_MAX_PSC_LIST + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_cell_search_request_geran_overrun()
{
	nfapi_cell_search_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_GERAN, &p, end); // rat_type

	push16(NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG, &p, end);
	push16(8, &p, end);

	push32(11, &p, end);
	push8(NFAPI_MAX_ARFCN_LIST + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_cell_search_indication_lte_overrun()
{
	nfapi_cell_search_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_LTE_CELL_SEARCH_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_LTE_CELLS_FOUND + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_cell_search_indication_utran_overrun()
{
	nfapi_cell_search_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_UTRAN_CELLS_FOUND + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_cell_search_indication_geran_overrun()
{
	nfapi_cell_search_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_GSM_CELLS_FOUND + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_cell_search_indication_state_overrun()
{
	nfapi_cell_search_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_CELL_SEARCH_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG, &p, end);
	push16(13, &p, end);

	push16(1, &p, end);
	if(1)
	{
		push16(42, &p, end);
		push8(42, &p, end);
		push8(42, &p, end);
		push8(42, &p, end);
		pushs16(42, &p, end);
		push32(42, &p, end);
	}

	push16(NFAPI_PNF_CELL_SEARCH_STATE_TAG, &p, end);
	push16(NFAPI_MAX_OPAQUE_DATA + 12, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_broadcast_detect_request_lte()
{
	nfapi_broadcast_detect_request_t in;
	nfapi_broadcast_detect_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.lte_broadcast_detect_request.tl.tag = NFAPI_LTE_BROADCAST_DETECT_REQUEST_TAG;
	in.lte_broadcast_detect_request.earfcn = 123;
	in.lte_broadcast_detect_request.pci = 12;
	in.lte_broadcast_detect_request.timeout = 1246;

	in.pnf_cell_search_state.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_request.earfcn, out.lte_broadcast_detect_request.earfcn);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_request.pci, out.lte_broadcast_detect_request.pci);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_request.timeout, out.lte_broadcast_detect_request.timeout);

}
void nfapi_test_broadcast_detect_request_utran()
{
	uint16_t idx;
	nfapi_broadcast_detect_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_broadcast_detect_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_UTRAN;

	in.utran_broadcast_detect_request.tl.tag = NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG;
	in.utran_broadcast_detect_request.uarfcn = 123;
	in.utran_broadcast_detect_request.psc = 12;
	in.utran_broadcast_detect_request.timeout = 1246;

	in.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	in.pnf_cell_search_state.length = 60;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, -1);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_request.uarfcn, out.utran_broadcast_detect_request.uarfcn);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_request.psc, out.utran_broadcast_detect_request.psc);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_request.timeout, out.utran_broadcast_detect_request.timeout);

	CU_ASSERT_EQUAL(in.pnf_cell_search_state.length, out.pnf_cell_search_state.length);

	for(idx = 0; idx < out.pnf_cell_search_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_search_state.value[idx], out.pnf_cell_search_state.value[idx]);
	}

}

void nfapi_test_broadcast_detect_request_nb_iot()
{
	uint16_t idx;
	nfapi_broadcast_detect_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_broadcast_detect_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_NB_IOT;

	in.nb_iot_broadcast_detect_request.tl.tag = NFAPI_NB_IOT_BROADCAST_DETECT_REQUEST_TAG;
	in.nb_iot_broadcast_detect_request.earfcn = 123;
	in.nb_iot_broadcast_detect_request.ro_dl = 12;
	in.nb_iot_broadcast_detect_request.pci = 12;
	in.nb_iot_broadcast_detect_request.timeout = 1246;

	in.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
	in.pnf_cell_search_state.length = 60;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, -1);

	int unpackResult = nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, 0);
	CU_ASSERT_NOT_EQUAL(unpackResult, -1);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_request.earfcn, out.nb_iot_broadcast_detect_request.earfcn);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_request.ro_dl, out.nb_iot_broadcast_detect_request.ro_dl);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_request.pci, out.nb_iot_broadcast_detect_request.pci);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_request.timeout, out.nb_iot_broadcast_detect_request.timeout);

	CU_ASSERT_EQUAL(in.pnf_cell_search_state.length, out.pnf_cell_search_state.length);

	for(idx = 0; idx < out.pnf_cell_search_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_search_state.value[idx], out.pnf_cell_search_state.value[idx]);
	}

}


void nfapi_test_broadcast_detect_request_state_overrun()
{
	nfapi_broadcast_detect_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_BROADCAST_DETECT_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_UTRAN, &p, end); // error_code

	push16(NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG, &p, end);
	push16(8, &p, end);

	push16(42, &p, end);
	push16(42, &p, end);
	push32(42, &p, end);

	push16(NFAPI_PNF_CELL_SEARCH_STATE_TAG, &p, end);
	push16(NFAPI_MAX_OPAQUE_DATA + 12, &p, end);


	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_broadcast_detect_response()
{
	nfapi_broadcast_detect_response_t in;
	nfapi_broadcast_detect_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);

}
void nfapi_test_broadcast_detect_indication_lte()
{
	uint16_t idx;
	nfapi_broadcast_detect_indication_t in;
	nfapi_broadcast_detect_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	in.lte_broadcast_detect_indication.tl.tag = NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG;
	in.lte_broadcast_detect_indication.number_of_tx_antenna = 1;
	in.lte_broadcast_detect_indication.mib_length = 8;
	in.lte_broadcast_detect_indication.sfn_offset = 4;
	in.utran_broadcast_detect_indication.tl.tag = 0;
	in.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	in.pnf_cell_broadcast_state.length = 23;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.tl.tag, out.lte_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.number_of_tx_antenna, out.lte_broadcast_detect_indication.number_of_tx_antenna);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.mib_length, out.lte_broadcast_detect_indication.mib_length);
	for(idx = 0; idx < out.lte_broadcast_detect_indication.mib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.mib[idx], out.lte_broadcast_detect_indication.mib[idx]);
	}
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.sfn_offset, out.lte_broadcast_detect_indication.sfn_offset);


	CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.tl.tag, out.utran_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
	for(idx = 0; idx < out.pnf_cell_broadcast_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.value[idx], out.pnf_cell_broadcast_state.value[idx]);
	}
}
void nfapi_test_broadcast_detect_indication_lte_overrun()
{
	nfapi_broadcast_detect_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_BROADCAST_DETECT_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push8(1, &p, end);
	push16(NFAPI_MAX_MIB_LENGTH + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out) ,0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_broadcast_detect_indication_utran()
{
	uint16_t idx;
	nfapi_broadcast_detect_indication_t in;
	nfapi_broadcast_detect_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	in.lte_broadcast_detect_indication.tl.tag = 0;
	in.utran_broadcast_detect_indication.tl.tag = NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG;
	in.utran_broadcast_detect_indication.mib_length = 16;
	in.utran_broadcast_detect_indication.sfn_offset = 4;
	in.pnf_cell_broadcast_state.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.tl.tag, out.lte_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.tl.tag, out.utran_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.mib_length, out.utran_broadcast_detect_indication.mib_length);
	for(idx = 0; idx < out.utran_broadcast_detect_indication.mib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.mib[idx], out.utran_broadcast_detect_indication.mib[idx]);
	}
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.sfn_offset, out.utran_broadcast_detect_indication.sfn_offset);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
}
void nfapi_test_broadcast_detect_indication_nb_iot()
{
	uint16_t idx;
	nfapi_broadcast_detect_indication_t in;
	nfapi_broadcast_detect_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_BROADCAST_DETECT_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	in.lte_broadcast_detect_indication.tl.tag = 0;
	in.utran_broadcast_detect_indication.tl.tag = 0;
	in.nb_iot_broadcast_detect_indication.tl.tag = NFAPI_NB_IOT_BROADCAST_DETECT_INDICATION_TAG;
	in.nb_iot_broadcast_detect_indication.number_of_tx_antenna = 2;
	in.nb_iot_broadcast_detect_indication.mib_length = 16;
	in.nb_iot_broadcast_detect_indication.sfn_offset = 4;
	in.pnf_cell_broadcast_state.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_broadcast_detect_indication.tl.tag, out.lte_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_broadcast_detect_indication.tl.tag, out.utran_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_indication.tl.tag, out.nb_iot_broadcast_detect_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_indication.number_of_tx_antenna, out.nb_iot_broadcast_detect_indication.number_of_tx_antenna);
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_indication.mib_length, out.nb_iot_broadcast_detect_indication.mib_length);
	for(idx = 0; idx < out.nb_iot_broadcast_detect_indication.mib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_indication.mib[idx], out.nb_iot_broadcast_detect_indication.mib[idx]);
	}
	CU_ASSERT_EQUAL(in.nb_iot_broadcast_detect_indication.sfn_offset, out.nb_iot_broadcast_detect_indication.sfn_offset);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
}
void nfapi_test_broadcast_detect_indication_utran_overrun()
{
	nfapi_broadcast_detect_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_BROADCAST_DETECT_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error_code

	push16(NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_MIB_LENGTH + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_broadcast_detect_indication_state_overrun()
{
}
void nfapi_test_system_information_schedule_request_lte()
{
	uint16_t idx;
	nfapi_system_information_schedule_request_t in;
	nfapi_system_information_schedule_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.nb_iot_system_information_schedule_request.tl.tag = 0;

	in.lte_system_information_schedule_request.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG;
	in.lte_system_information_schedule_request.earfcn = 16;
	in.lte_system_information_schedule_request.pci = 4;
	in.lte_system_information_schedule_request.downlink_channel_bandwidth = 20;
	in.lte_system_information_schedule_request.phich_configuration = 1;
	in.lte_system_information_schedule_request.number_of_tx_antenna = 2;
	in.lte_system_information_schedule_request.retry_count = 4;
	in.lte_system_information_schedule_request.timeout = 0;
	in.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	in.pnf_cell_broadcast_state.length = 7;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.tl.tag, out.lte_system_information_schedule_request.tl.tag);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.earfcn, out.lte_system_information_schedule_request.earfcn);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.pci, out.lte_system_information_schedule_request.pci);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.downlink_channel_bandwidth, out.lte_system_information_schedule_request.downlink_channel_bandwidth);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.downlink_channel_bandwidth, out.lte_system_information_schedule_request.downlink_channel_bandwidth);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.number_of_tx_antenna, out.lte_system_information_schedule_request.number_of_tx_antenna);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.retry_count, out.lte_system_information_schedule_request.retry_count);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.timeout, out.lte_system_information_schedule_request.timeout);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.length, out.pnf_cell_broadcast_state.length);
	for(idx = 0; idx < out.pnf_cell_broadcast_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.value[idx], out.pnf_cell_broadcast_state.value[idx]);
	}
}

void nfapi_test_system_information_schedule_request_nb_iot()
{
	uint16_t idx;
	nfapi_system_information_schedule_request_t in;
	nfapi_system_information_schedule_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_NB_IOT;

	in.lte_system_information_schedule_request.tl.tag = 0;
	in.nb_iot_system_information_schedule_request.tl.tag = NFAPI_NB_IOT_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG;
	in.nb_iot_system_information_schedule_request.earfcn = 16;
	in.nb_iot_system_information_schedule_request.ro_dl = 4;
	in.nb_iot_system_information_schedule_request.pci = 4;
	in.nb_iot_system_information_schedule_request.scheduling_info_sib1_nb = 4;
	in.nb_iot_system_information_schedule_request.timeout = 0;
	in.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	in.pnf_cell_broadcast_state.length = 7;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_system_information_schedule_request.tl.tag, out.lte_system_information_schedule_request.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.tl.tag, out.nb_iot_system_information_schedule_request.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.earfcn, out.nb_iot_system_information_schedule_request.earfcn);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.ro_dl, out.nb_iot_system_information_schedule_request.ro_dl);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.pci, out.nb_iot_system_information_schedule_request.pci);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.scheduling_info_sib1_nb, out.nb_iot_system_information_schedule_request.scheduling_info_sib1_nb);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_schedule_request.timeout, out.nb_iot_system_information_schedule_request.timeout);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.length, out.pnf_cell_broadcast_state.length);
	for(idx = 0; idx < out.pnf_cell_broadcast_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.value[idx], out.pnf_cell_broadcast_state.value[idx]);
	}
}

void nfapi_test_system_information_schedule_request_state_overrun()
{
	nfapi_system_information_schedule_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(0, &p, end); // rat type

	push16(NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG, &p, end);
	push16(13, &p, end);

	push16(1, &p, end);
	push16(1, &p, end);
	push16(1, &p, end);
	push8(142, &p, end);
	push8(142, &p, end);
	push8(142, &p, end);
	push32(42, &p, end);

	push16(NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &p, end);
	push16(NFAPI_MAX_OPAQUE_DATA + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);

}
void nfapi_test_system_information_schedule_response()
{
	nfapi_system_information_schedule_response_t in;
	nfapi_system_information_schedule_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_system_information_schedule_indication_lte()
{
	uint16_t idx;
	nfapi_system_information_schedule_indication_t in;
	nfapi_system_information_schedule_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = 0;

	in.nb_iot_system_information_indication.tl.tag = 0;
	in.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
	in.lte_system_information_indication.sib_type = 16;
	in.lte_system_information_indication.sib_length = 4;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.sib_type, out.lte_system_information_indication.sib_type);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.sib_length, out.lte_system_information_indication.sib_length);

	for(idx = 0; idx < out.lte_system_information_indication.sib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_system_information_indication.sib[idx], out.lte_system_information_indication.sib[idx]);
	}
}
void nfapi_test_system_information_schedule_indication_nb_iot()
{
	uint16_t idx;
	nfapi_system_information_schedule_indication_t in;
	nfapi_system_information_schedule_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = 0;

	in.lte_system_information_indication.tl.tag = 0;
	in.nb_iot_system_information_indication.tl.tag = NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG;
	in.nb_iot_system_information_indication.sib_type = 16;
	in.nb_iot_system_information_indication.sib_length = 4;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.tl.tag, out.nb_iot_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib_type, out.nb_iot_system_information_indication.sib_type);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib_length, out.nb_iot_system_information_indication.sib_length);

	for(idx = 0; idx < out.nb_iot_system_information_indication.sib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib[idx], out.nb_iot_system_information_indication.sib[idx]);
	}
}
void nfapi_test_system_information_request_lte()
{
	uint16_t idx;
	nfapi_system_information_request_t in;
	nfapi_system_information_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_LTE;

	in.lte_system_information_request.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG;
	in.lte_system_information_request.earfcn = 16;
	in.lte_system_information_request.pci = 4;
	in.lte_system_information_request.downlink_channel_bandwidth = 20;
	in.lte_system_information_request.phich_configuration = 1;
	in.lte_system_information_request.number_of_tx_antenna = 2;
	in.lte_system_information_request.number_of_si_periodicity = 1;
	in.lte_system_information_request.si_periodicity[0].si_periodicity = 3;
	in.lte_system_information_request.si_periodicity[0].si_index = 0;
	in.lte_system_information_request.si_window_length = 0;
	in.lte_system_information_request.timeout = 2000;

	in.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	in.pnf_cell_broadcast_state.length = 7;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.lte_system_information_request.tl.tag, out.lte_system_information_request.tl.tag);
	CU_ASSERT_EQUAL(in.lte_system_information_request.earfcn, out.lte_system_information_request.earfcn);
	CU_ASSERT_EQUAL(in.lte_system_information_request.pci, out.lte_system_information_request.pci);
	CU_ASSERT_EQUAL(in.lte_system_information_request.downlink_channel_bandwidth, out.lte_system_information_request.downlink_channel_bandwidth);
	CU_ASSERT_EQUAL(in.lte_system_information_request.downlink_channel_bandwidth, out.lte_system_information_request.downlink_channel_bandwidth);
	CU_ASSERT_EQUAL(in.lte_system_information_request.number_of_tx_antenna, out.lte_system_information_request.number_of_tx_antenna);
	CU_ASSERT_EQUAL(in.lte_system_information_request.number_of_si_periodicity, out.lte_system_information_request.number_of_si_periodicity);
	for(idx = 0; idx < in.lte_system_information_request.number_of_si_periodicity; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_system_information_request.si_periodicity[idx].si_periodicity, out.lte_system_information_request.si_periodicity[idx].si_periodicity);
		CU_ASSERT_EQUAL(in.lte_system_information_request.si_periodicity[idx].si_index, out.lte_system_information_request.si_periodicity[idx].si_index);
	}

	CU_ASSERT_EQUAL(in.lte_system_information_request.si_window_length, out.lte_system_information_request.si_window_length);
	CU_ASSERT_EQUAL(in.lte_system_information_request.timeout, out.lte_system_information_request.timeout);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.length, out.pnf_cell_broadcast_state.length);
	for(idx = 0; idx < out.pnf_cell_broadcast_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.value[idx], out.pnf_cell_broadcast_state.value[idx]);
	}
}
void nfapi_test_system_information_request_lte_overrun()
{
	nfapi_system_information_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_LTE, &p, end); 

	push16(NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG, &p, end);
	push16(8, &p, end);

	push16(8, &p, end);
	push16(8, &p, end);
	push16(8, &p, end);
	push8(8, &p, end);
	push8(8, &p, end);
	push8(NFAPI_MAX_SI_PERIODICITY + 1, &p, end);


	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_system_information_request_utran()
{
	nfapi_system_information_request_t in;
	nfapi_system_information_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_UTRAN;

	in.utran_system_information_request.tl.tag = NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG;
	in.utran_system_information_request.uarfcn = 16;
	in.utran_system_information_request.psc = 4;
	in.utran_system_information_request.timeout = 2000;

	in.pnf_cell_broadcast_state.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.utran_system_information_request.tl.tag, out.utran_system_information_request.tl.tag);
	CU_ASSERT_EQUAL(in.utran_system_information_request.uarfcn, out.utran_system_information_request.uarfcn);
	CU_ASSERT_EQUAL(in.utran_system_information_request.psc, out.utran_system_information_request.psc);
	CU_ASSERT_EQUAL(in.utran_system_information_request.timeout, out.utran_system_information_request.timeout);

	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
}
void nfapi_test_system_information_request_geran()
{
	nfapi_system_information_request_t in;
	nfapi_system_information_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_GERAN;

	in.geran_system_information_request.tl.tag = NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG;
	in.geran_system_information_request.arfcn = 16;
	in.geran_system_information_request.bsic = 4;
	in.geran_system_information_request.timeout = 2000;

	in.pnf_cell_broadcast_state.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rat_type, out.rat_type);
	CU_ASSERT_EQUAL(in.geran_system_information_request.tl.tag, out.geran_system_information_request.tl.tag);
	CU_ASSERT_EQUAL(in.geran_system_information_request.arfcn, out.geran_system_information_request.arfcn);
	CU_ASSERT_EQUAL(in.geran_system_information_request.bsic, out.geran_system_information_request.bsic);
	CU_ASSERT_EQUAL(in.geran_system_information_request.timeout, out.geran_system_information_request.timeout);

	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
}
void nfapi_test_system_information_request_state_overrun()
{
	nfapi_system_information_request_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_REQUEST, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push8(NFAPI_RAT_TYPE_LTE, &p, end); 

	push16(NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG, &p, end);
	push16(16, &p, end);

	push16(8, &p, end);
	push16(8, &p, end);
	push16(8, &p, end);
	push8(8, &p, end);
	push8(8, &p, end);
	push8(1, &p, end);
	{
		push8(1, &p, end);
		push8(1, &p, end);
	}
	push8(1, &p, end);
	push32(1, &p, end);

	push16(NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &p, end);
	push16(NFAPI_MAX_OPAQUE_DATA + 1, &p, end);

	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_system_information_request_nb_iot()
{
	uint16_t idx, idx2;
	nfapi_system_information_request_t in;
	nfapi_system_information_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.rat_type = NFAPI_RAT_TYPE_NB_IOT;

	in.nb_iot_system_information_request.tl.tag = NFAPI_NB_IOT_SYSTEM_INFORMATION_REQUEST_TAG;
	in.nb_iot_system_information_request.earfcn = 16;
	in.nb_iot_system_information_request.ro_dl = 1;
	in.nb_iot_system_information_request.pci = 4;
	in.nb_iot_system_information_request.number_of_si_periodicity = 1;
	in.nb_iot_system_information_request.si_periodicity[0].si_periodicity = 3;
	in.nb_iot_system_information_request.si_periodicity[0].si_repetition_pattern = 0;
	in.nb_iot_system_information_request.si_periodicity[0].si_tb_size = 0;
	in.nb_iot_system_information_request.si_periodicity[0].number_of_si_index = 2;
	in.nb_iot_system_information_request.si_periodicity[0].si_index[0] = 2;
	in.nb_iot_system_information_request.si_periodicity[0].si_index[1] = 20;
	in.nb_iot_system_information_request.si_window_length = 0;
	in.nb_iot_system_information_request.timeout = 2000;

	in.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
	in.pnf_cell_broadcast_state.length = 7;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	IN_OUT_ASSERT(header.message_id);
	IN_OUT_ASSERT(rat_type);
	IN_OUT_ASSERT(nb_iot_system_information_request.tl.tag);
	IN_OUT_ASSERT(nb_iot_system_information_request.earfcn);
	IN_OUT_ASSERT(nb_iot_system_information_request.ro_dl);
	IN_OUT_ASSERT(nb_iot_system_information_request.pci);
	IN_OUT_ASSERT(nb_iot_system_information_request.number_of_si_periodicity);
	
	for(idx = 0; idx < in.nb_iot_system_information_request.number_of_si_periodicity; ++idx)
	{
		IN_OUT_ASSERT(nb_iot_system_information_request.si_periodicity[idx].si_periodicity);
		IN_OUT_ASSERT(nb_iot_system_information_request.si_periodicity[idx].si_repetition_pattern);
		IN_OUT_ASSERT(nb_iot_system_information_request.si_periodicity[idx].si_tb_size);
		IN_OUT_ASSERT(nb_iot_system_information_request.si_periodicity[idx].number_of_si_index);
		
		for(idx2 = 0; idx2 < in.nb_iot_system_information_request.si_periodicity[idx].number_of_si_index; ++idx2)
			IN_OUT_ASSERT(nb_iot_system_information_request.si_periodicity[idx].si_index[idx2]);
	}

	IN_OUT_ASSERT(nb_iot_system_information_request.si_window_length);
	IN_OUT_ASSERT(nb_iot_system_information_request.timeout);


	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.tl.tag, out.pnf_cell_broadcast_state.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.length, out.pnf_cell_broadcast_state.length);
	for(idx = 0; idx < out.pnf_cell_broadcast_state.length; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_cell_broadcast_state.value[idx], out.pnf_cell_broadcast_state.value[idx]);
	}
}


void nfapi_test_system_information_response()
{
	nfapi_system_information_response_t in;
	nfapi_system_information_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_system_information_indication_lte()
{
	uint16_t idx;
	nfapi_system_information_indication_t in;
	nfapi_system_information_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;
	in.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
	in.lte_system_information_indication.sib_type = 2;
	in.lte_system_information_indication.sib_length = 15;

	in.utran_system_information_indication.tl.tag = 0;
	in.geran_system_information_indication.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.sib_type, out.lte_system_information_indication.sib_type);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.sib_length, out.lte_system_information_indication.sib_length);

	for(idx = 0; idx < out.lte_system_information_indication.sib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.lte_system_information_indication.sib[idx], out.lte_system_information_indication.sib[idx]);
	}

	CU_ASSERT_EQUAL(in.utran_system_information_indication.tl.tag, out.utran_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_system_information_indication.tl.tag, out.geran_system_information_indication.tl.tag);

}
void nfapi_test_system_information_indication_lte_overrun()
{
	nfapi_system_information_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error code

	push16(NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push8(1, &p, end);
	push16(NFAPI_MAX_SIB_LENGTH + 1, &p, end);


	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_system_information_indication_utran()
{
	uint16_t idx;
	nfapi_system_information_indication_t in;
	nfapi_system_information_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;
	in.lte_system_information_indication.tl.tag = 0;

	in.utran_system_information_indication.tl.tag = NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG;
	in.utran_system_information_indication.sib_length = 25;

	in.geran_system_information_indication.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_system_information_indication.tl.tag, out.utran_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_system_information_indication.sib_length, out.utran_system_information_indication.sib_length);

	for(idx = 0; idx < out.utran_system_information_indication.sib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.utran_system_information_indication.sib[idx], out.utran_system_information_indication.sib[idx]);
	}

	CU_ASSERT_EQUAL(in.geran_system_information_indication.tl.tag, out.geran_system_information_indication.tl.tag);

}
void nfapi_test_system_information_indication_utran_overrun()
{
	nfapi_system_information_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error code

	push16(NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_SIB_LENGTH + 1, &p, end);


	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}
void nfapi_test_system_information_indication_geran()
{
	uint16_t idx;
	nfapi_system_information_indication_t in;
	nfapi_system_information_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;
	in.lte_system_information_indication.tl.tag = 0;
	in.utran_system_information_indication.tl.tag = 0;

	in.geran_system_information_indication.tl.tag = NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG;
	in.geran_system_information_indication.si_length = 25;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_system_information_indication.tl.tag, out.utran_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_system_information_indication.tl.tag, out.geran_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_system_information_indication.si_length, out.geran_system_information_indication.si_length);

	for(idx = 0; idx < out.geran_system_information_indication.si_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.geran_system_information_indication.si[idx], out.geran_system_information_indication.si[idx]);
	}

}
void nfapi_test_system_information_indication_geran_overrun()
{
	nfapi_system_information_indication_t out;

	uint8_t buffer[1024];
	uint8_t* p = &buffer[0];
	uint8_t *end = p + 1024;
	push16(0, &p, end);
	push16(NFAPI_SYSTEM_INFORMATION_INDICATION, &p, end);
	push16(64, &p, end);
	push16(0, &p, end);

	push32(0, &p, end); // error code

	push16(NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG, &p, end);
	push16(8, &p, end);

	push16(NFAPI_MAX_SI_LENGTH + 1, &p, end);


	int result = nfapi_p4_message_unpack(buffer, 34, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(result, -1);
}

void nfapi_test_system_information_indication_nb_iot()
{
	uint16_t idx;
	nfapi_system_information_indication_t in;
	nfapi_system_information_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;
	in.nb_iot_system_information_indication.tl.tag = NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG;
	in.nb_iot_system_information_indication.sib_type = 2;
	in.nb_iot_system_information_indication.sib_length = 15;

	in.lte_system_information_indication.tl.tag = 0;
	in.utran_system_information_indication.tl.tag = 0;
	in.geran_system_information_indication.tl.tag = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.tl.tag, out.nb_iot_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib_type, out.nb_iot_system_information_indication.sib_type);
	CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib_length, out.nb_iot_system_information_indication.sib_length);

	for(idx = 0; idx < out.nb_iot_system_information_indication.sib_length; ++idx)
	{
		CU_ASSERT_EQUAL(in.nb_iot_system_information_indication.sib[idx], out.nb_iot_system_information_indication.sib[idx]);
	}

	CU_ASSERT_EQUAL(in.lte_system_information_indication.tl.tag, out.lte_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.utran_system_information_indication.tl.tag, out.utran_system_information_indication.tl.tag);
	CU_ASSERT_EQUAL(in.geran_system_information_indication.tl.tag, out.geran_system_information_indication.tl.tag);

}

void nfapi_test_nmm_stop_request()
{
	nfapi_nmm_stop_request_t in;
	nfapi_nmm_stop_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_NMM_STOP_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}
void nfapi_test_nmm_stop_response()
{
	nfapi_nmm_stop_response_t in;
	nfapi_nmm_stop_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_NMM_STOP_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_P4_MSG_RAT_NOT_SUPPORTED;

	int packedMessageLength = nfapi_p4_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p4_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}


void nfapi_test_pnf_param_request()
{
	nfapi_pnf_param_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_param_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_PARAM_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}
void nfapi_test_pnf_param_request_ve()
{
	nfapi_pnf_param_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_param_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_PARAM_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	my_vendor_extention ve;
	ve.tl.tag = NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE;
	ve.value1 = 56;
	ve.value2 = 1234;
	in.vendor_extension = (void*)&ve;

	nfapi_p4_p5_codec_config_t codec;
	memset(&codec, 0, sizeof(codec));
	codec.unpack_vendor_extension_tlv = &nfapi_test_unpack_vendor_extension_tlv; 
	codec.pack_vendor_extension_tlv = &nfapi_test_pack_vendor_extension_tlv;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, &codec);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), &codec);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

	CU_ASSERT_NOT_EQUAL(in.vendor_extension, 0);
	CU_ASSERT_EQUAL(((my_vendor_extention*)(out.vendor_extension))->tl.tag, ve.tl.tag);
	CU_ASSERT_EQUAL(((my_vendor_extention*)(out.vendor_extension))->value1, ve.value1);
	CU_ASSERT_EQUAL(((my_vendor_extention*)(out.vendor_extension))->value2, ve.value2);

	free(out.vendor_extension);
}
void nfapi_test_pnf_param_response()
{
	uint16_t idx = 0;
	uint16_t idx2 = 0;
	nfapi_pnf_param_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_param_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = 0;
	in.pnf_param_general.tl.tag = NFAPI_PNF_PARAM_GENERAL_TAG;
	in.pnf_param_general.nfapi_sync_mode = 2;
	in.pnf_param_general.location_mode = 1;
	in.pnf_param_general.location_coordinates_length = 1;
	in.pnf_param_general.location_coordinates[0] = 123;
	in.pnf_param_general.dl_config_timing = 12;
	in.pnf_param_general.tx_timing = 12;
	in.pnf_param_general.ul_config_timing = 12;
	in.pnf_param_general.hi_dci0_timing = 12;
	in.pnf_param_general.maximum_number_phys = 2;
	in.pnf_param_general.maximum_total_bandwidth = 100;
	in.pnf_param_general.maximum_total_number_dl_layers = 2;
	in.pnf_param_general.maximum_total_number_ul_layers = 2;
	in.pnf_param_general.shared_bands = 0;
	in.pnf_param_general.shared_pa = 0;
	in.pnf_param_general.maximum_total_power = -190;
	in.pnf_param_general.oui[0] = 88;

	in.pnf_phy.tl.tag = NFAPI_PNF_PHY_TAG;
	in.pnf_phy.number_of_phys = 2;
	in.pnf_phy.phy[0].phy_config_index = 0;
	in.pnf_phy.phy[0].number_of_rfs = 2;
	in.pnf_phy.phy[0].rf_config[0].rf_config_index = 0;
	in.pnf_phy.phy[0].number_of_rf_exclusions = 1;
	in.pnf_phy.phy[0].rf_config[0].rf_config_index = 1;
	in.pnf_phy.phy[0].downlink_channel_bandwidth_supported = 20;
	in.pnf_phy.phy[0].uplink_channel_bandwidth_supported = 20;
	in.pnf_phy.phy[0].number_of_dl_layers_supported = 2;
	in.pnf_phy.phy[0].number_of_ul_layers_supported = 1;
	in.pnf_phy.phy[0].maximum_3gpp_release_supported = 3;
	in.pnf_phy.phy[0].nmm_modes_supported = 2;

	in.pnf_rf.tl.tag = NFAPI_PNF_RF_TAG;
	in.pnf_rf.number_of_rfs = 2;
	in.pnf_rf.rf[0].rf_config_index = 0;
	in.pnf_rf.rf[0].band = 1;
	in.pnf_rf.rf[0].maximum_transmit_power = -100; 
	in.pnf_rf.rf[0].minimum_transmit_power = -90; 
	in.pnf_rf.rf[0].number_of_antennas_suppported = 4;
	in.pnf_rf.rf[0].minimum_downlink_frequency = 100;
	in.pnf_rf.rf[0].maximum_downlink_frequency = 2132;
	in.pnf_rf.rf[0].minimum_uplink_frequency = 1231;
	in.pnf_rf.rf[0].maximum_uplink_frequency = 123;


	in.pnf_phy_rel10.tl.tag = NFAPI_PNF_PHY_REL10_TAG;
	in.pnf_phy_rel10.number_of_phys = 2;
	in.pnf_phy_rel10.phy[0].phy_config_index = 0;
	in.pnf_phy_rel10.phy[0].transmission_mode_7_supported = 1;
	in.pnf_phy_rel10.phy[0].transmission_mode_8_supported = 2;
	in.pnf_phy_rel10.phy[0].two_antenna_ports_for_pucch = 3;
	in.pnf_phy_rel10.phy[0].transmission_mode_9_supported = 4;
	in.pnf_phy_rel10.phy[0].simultaneous_pucch_pusch = 5;
	in.pnf_phy_rel10.phy[0].four_layer_tx_with_tm3_and_tm4 = 6;

	in.pnf_phy_rel11.tl.tag = NFAPI_PNF_PHY_REL11_TAG;
	in.pnf_phy_rel11.number_of_phys = 2;
	in.pnf_phy_rel11.phy[0].phy_config_index = 0;
	in.pnf_phy_rel11.phy[0].edpcch_supported = 1;
	in.pnf_phy_rel11.phy[0].multi_ack_csi_reporting = 2;
	in.pnf_phy_rel11.phy[0].pucch_tx_diversity = 3;
	in.pnf_phy_rel11.phy[0].ul_comp_supported = 4;
	in.pnf_phy_rel11.phy[0].transmission_mode_5_supported = 5;

	in.pnf_phy_rel12.tl.tag = NFAPI_PNF_PHY_REL12_TAG;
	in.pnf_phy_rel12.number_of_phys = 2;
	in.pnf_phy_rel12.phy[0].phy_config_index = 0;
	in.pnf_phy_rel12.phy[0].csi_subframe_set = 1;
	in.pnf_phy_rel12.phy[0].enhanced_4tx_codebook = 2;
	in.pnf_phy_rel12.phy[0].drs_supported = 3;
	in.pnf_phy_rel12.phy[0].ul_64qam_supported = 4;
	in.pnf_phy_rel12.phy[0].transmission_mode_10_supported = 5;
	in.pnf_phy_rel12.phy[0].alternative_bts_indices = 6;

	in.pnf_phy_rel13.tl.tag = NFAPI_PNF_PHY_REL13_TAG;
	in.pnf_phy_rel13.number_of_phys = 2;
	in.pnf_phy_rel13.phy[0].phy_config_index = 0;
	in.pnf_phy_rel13.phy[0].pucch_format4_supported = 1;
	in.pnf_phy_rel13.phy[0].pucch_format5_supported = 2;
	in.pnf_phy_rel13.phy[0].more_than_5_ca_support = 3;
	in.pnf_phy_rel13.phy[0].laa_supported = 4;
	in.pnf_phy_rel13.phy[0].laa_ending_in_dwpts_supported = 5;
	in.pnf_phy_rel13.phy[0].laa_starting_in_second_slot_supported = 6;
	in.pnf_phy_rel13.phy[0].beamforming_supported = 7;
	in.pnf_phy_rel13.phy[0].csi_rs_enhancement_supported = 8;
	in.pnf_phy_rel13.phy[0].drms_enhancement_supported = 9;
	in.pnf_phy_rel13.phy[0].srs_enhancement_supported = 10;
	
	in.pnf_phy_rel13_nb_iot.tl.tag = NFAPI_PNF_PHY_REL13_NB_IOT_TAG;
	in.pnf_phy_rel13_nb_iot.number_of_phys = 2;
	in.pnf_phy_rel13_nb_iot.phy[0].phy_config_index = 0;
	in.pnf_phy_rel13_nb_iot.phy[0].number_of_rfs = 2;
	in.pnf_phy_rel13_nb_iot.phy[0].rf_config[0].rf_config_index = 0;
	in.pnf_phy_rel13_nb_iot.phy[0].number_of_rf_exclusions = 1;
	in.pnf_phy_rel13_nb_iot.phy[0].rf_config[0].rf_config_index = 1;
	in.pnf_phy_rel13_nb_iot.phy[0].number_of_dl_layers_supported = 2;
	in.pnf_phy_rel13_nb_iot.phy[0].number_of_ul_layers_supported = 1;
	in.pnf_phy_rel13_nb_iot.phy[0].maximum_3gpp_release_supported = 3;
	in.pnf_phy_rel13_nb_iot.phy[0].nmm_modes_supported = 2;


	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpack_result = nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(unpack_result, 1);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.pnf_param_general.tl.tag, out.pnf_param_general.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_param_general.nfapi_sync_mode, out.pnf_param_general.nfapi_sync_mode);
	CU_ASSERT_EQUAL(in.pnf_param_general.location_mode, out.pnf_param_general.location_mode);

	for(idx = 0; idx < NFAPI_PNF_PARAM_GENERAL_LOCATION_LENGTH; ++idx)
		CU_ASSERT_EQUAL(in.pnf_param_general.location_coordinates[idx], out.pnf_param_general.location_coordinates[idx]);

	CU_ASSERT_EQUAL(in.pnf_param_general.dl_config_timing, out.pnf_param_general.dl_config_timing);
	CU_ASSERT_EQUAL(in.pnf_param_general.tx_timing, out.pnf_param_general.tx_timing);
	CU_ASSERT_EQUAL(in.pnf_param_general.ul_config_timing, out.pnf_param_general.ul_config_timing);
	CU_ASSERT_EQUAL(in.pnf_param_general.hi_dci0_timing, out.pnf_param_general.hi_dci0_timing);
	CU_ASSERT_EQUAL(in.pnf_param_general.maximum_number_phys, out.pnf_param_general.maximum_number_phys);
	CU_ASSERT_EQUAL(in.pnf_param_general.maximum_total_bandwidth, out.pnf_param_general.maximum_total_bandwidth);
	CU_ASSERT_EQUAL(in.pnf_param_general.maximum_total_number_dl_layers, out.pnf_param_general.maximum_total_number_dl_layers);
	CU_ASSERT_EQUAL(in.pnf_param_general.maximum_total_number_ul_layers, out.pnf_param_general.maximum_total_number_ul_layers);
	CU_ASSERT_EQUAL(in.pnf_param_general.shared_bands, out.pnf_param_general.shared_bands);
	CU_ASSERT_EQUAL(in.pnf_param_general.shared_pa, out.pnf_param_general.shared_pa);
	CU_ASSERT_EQUAL(in.pnf_param_general.maximum_total_power, out.pnf_param_general.maximum_total_power);
	for(idx = 0; idx < NFAPI_PNF_PARAM_GENERAL_OUI_LENGTH; ++idx)
		CU_ASSERT_EQUAL(in.pnf_param_general.oui[idx], out.pnf_param_general.oui[idx]);

	CU_ASSERT_EQUAL(in.pnf_phy.tl.tag, out.pnf_phy.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy.number_of_phys, out.pnf_phy.number_of_phys);
	
	for(idx = 0; idx < out.pnf_phy.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].phy_config_index, out.pnf_phy.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].number_of_rfs, out.pnf_phy.phy[idx].number_of_rfs);

	
		for(idx2 = 0; idx2 < out.pnf_phy.phy[idx].number_of_rfs; ++idx2)
		{
			CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].rf_config[idx2].rf_config_index, out.pnf_phy.phy[idx].rf_config[idx2].rf_config_index);
		}
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].number_of_rf_exclusions, out.pnf_phy.phy[idx].number_of_rf_exclusions);

		for(idx2 = 0; idx2 < out.pnf_phy.phy[idx].number_of_rf_exclusions; ++idx2)
		{
			CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].excluded_rf_config[idx2].rf_config_index, out.pnf_phy.phy[idx].excluded_rf_config[idx2].rf_config_index);
		}


		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].downlink_channel_bandwidth_supported, out.pnf_phy.phy[idx].downlink_channel_bandwidth_supported);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].uplink_channel_bandwidth_supported, out.pnf_phy.phy[idx].uplink_channel_bandwidth_supported);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].number_of_dl_layers_supported, out.pnf_phy.phy[idx].number_of_dl_layers_supported);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].number_of_ul_layers_supported, out.pnf_phy.phy[idx].number_of_ul_layers_supported);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].maximum_3gpp_release_supported, out.pnf_phy.phy[idx].maximum_3gpp_release_supported);
		CU_ASSERT_EQUAL(in.pnf_phy.phy[idx].nmm_modes_supported, out.pnf_phy.phy[idx].nmm_modes_supported);
	}

	CU_ASSERT_EQUAL(in.pnf_rf.tl.tag, out.pnf_rf.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_rf.number_of_rfs, out.pnf_rf.number_of_rfs);
	for(idx = 0; idx < out.pnf_rf.number_of_rfs; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].rf_config_index, out.pnf_rf.rf[idx].rf_config_index);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].band, out.pnf_rf.rf[idx].band);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].maximum_transmit_power, out.pnf_rf.rf[idx].maximum_transmit_power);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].minimum_transmit_power, out.pnf_rf.rf[idx].minimum_transmit_power);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].number_of_antennas_suppported, out.pnf_rf.rf[idx].number_of_antennas_suppported);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].minimum_downlink_frequency, out.pnf_rf.rf[idx].minimum_downlink_frequency);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].maximum_downlink_frequency, out.pnf_rf.rf[idx].maximum_downlink_frequency);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].minimum_uplink_frequency, out.pnf_rf.rf[idx].minimum_uplink_frequency);
		CU_ASSERT_EQUAL(in.pnf_rf.rf[idx].maximum_uplink_frequency, out.pnf_rf.rf[idx].maximum_uplink_frequency);
	}


	CU_ASSERT_EQUAL(in.pnf_phy_rel10.tl.tag, out.pnf_phy_rel10.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rel10.number_of_phys, out.pnf_phy_rel10.number_of_phys);
	for(idx = 0; idx < out.pnf_phy_rel10.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].phy_config_index, out.pnf_phy_rel10.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].transmission_mode_7_supported, out.pnf_phy_rel10.phy[idx].transmission_mode_7_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].transmission_mode_8_supported, out.pnf_phy_rel10.phy[idx].transmission_mode_8_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].two_antenna_ports_for_pucch, out.pnf_phy_rel10.phy[idx].two_antenna_ports_for_pucch);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].transmission_mode_9_supported, out.pnf_phy_rel10.phy[idx].transmission_mode_9_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].simultaneous_pucch_pusch, out.pnf_phy_rel10.phy[idx].simultaneous_pucch_pusch);
		CU_ASSERT_EQUAL(in.pnf_phy_rel10.phy[idx].four_layer_tx_with_tm3_and_tm4, out.pnf_phy_rel10.phy[idx].four_layer_tx_with_tm3_and_tm4);
	}

	CU_ASSERT_EQUAL(in.pnf_phy_rel11.tl.tag, out.pnf_phy_rel11.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rel11.number_of_phys, out.pnf_phy_rel11.number_of_phys);
	for(idx = 0; idx < out.pnf_phy_rel11.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].phy_config_index, out.pnf_phy_rel11.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].edpcch_supported, out.pnf_phy_rel11.phy[idx].edpcch_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].multi_ack_csi_reporting, out.pnf_phy_rel11.phy[idx].multi_ack_csi_reporting);
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].pucch_tx_diversity, out.pnf_phy_rel11.phy[idx].pucch_tx_diversity);
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].ul_comp_supported, out.pnf_phy_rel11.phy[idx].ul_comp_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel11.phy[idx].transmission_mode_5_supported, out.pnf_phy_rel11.phy[idx].transmission_mode_5_supported);
	}

	CU_ASSERT_EQUAL(in.pnf_phy_rel12.tl.tag, out.pnf_phy_rel12.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rel12.number_of_phys, out.pnf_phy_rel12.number_of_phys);
	for(idx = 0; idx < out.pnf_phy_rel12.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].phy_config_index, out.pnf_phy_rel12.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].csi_subframe_set, out.pnf_phy_rel12.phy[idx].csi_subframe_set);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].enhanced_4tx_codebook, out.pnf_phy_rel12.phy[idx].enhanced_4tx_codebook);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].drs_supported, out.pnf_phy_rel12.phy[idx].drs_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].ul_64qam_supported, out.pnf_phy_rel12.phy[idx].ul_64qam_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].transmission_mode_10_supported, out.pnf_phy_rel12.phy[idx].transmission_mode_10_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel12.phy[idx].alternative_bts_indices, out.pnf_phy_rel12.phy[idx].alternative_bts_indices);
	}

	CU_ASSERT_EQUAL(in.pnf_phy_rel13.tl.tag, out.pnf_phy_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rel13.number_of_phys, out.pnf_phy_rel13.number_of_phys);
	for(idx = 0; idx < out.pnf_phy_rel13.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].phy_config_index, out.pnf_phy_rel13.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].pucch_format4_supported, out.pnf_phy_rel13.phy[idx].pucch_format4_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].pucch_format5_supported, out.pnf_phy_rel13.phy[idx].pucch_format5_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].more_than_5_ca_support, out.pnf_phy_rel13.phy[idx].more_than_5_ca_support);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].laa_supported, out.pnf_phy_rel13.phy[idx].laa_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].laa_ending_in_dwpts_supported, out.pnf_phy_rel13.phy[idx].laa_ending_in_dwpts_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].laa_starting_in_second_slot_supported, out.pnf_phy_rel13.phy[idx].laa_starting_in_second_slot_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].beamforming_supported, out.pnf_phy_rel13.phy[idx].beamforming_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].csi_rs_enhancement_supported, out.pnf_phy_rel13.phy[idx].csi_rs_enhancement_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].drms_enhancement_supported, out.pnf_phy_rel13.phy[idx].drms_enhancement_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13.phy[idx].srs_enhancement_supported, out.pnf_phy_rel13.phy[idx].srs_enhancement_supported);
	}

	CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.tl.tag, out.pnf_phy_rel13_nb_iot.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.number_of_phys, out.pnf_phy_rel13_nb_iot.number_of_phys);
	
	for(idx = 0; idx < out.pnf_phy_rel13_nb_iot.number_of_phys; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].phy_config_index, out.pnf_phy_rel13_nb_iot.phy[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].number_of_rfs, out.pnf_phy_rel13_nb_iot.phy[idx].number_of_rfs);

	
		for(idx2 = 0; idx2 < out.pnf_phy_rel13_nb_iot.phy[idx].number_of_rfs; ++idx2)
		{
			CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].rf_config[idx2].rf_config_index, out.pnf_phy_rel13_nb_iot.phy[idx].rf_config[idx2].rf_config_index);
		}
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].number_of_rf_exclusions, out.pnf_phy_rel13_nb_iot.phy[idx].number_of_rf_exclusions);

		for(idx2 = 0; idx2 < out.pnf_phy_rel13_nb_iot.phy[idx].number_of_rf_exclusions; ++idx2)
		{
			CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].excluded_rf_config[idx2].rf_config_index, out.pnf_phy_rel13_nb_iot.phy[idx].excluded_rf_config[idx2].rf_config_index);
		}


		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].number_of_dl_layers_supported, out.pnf_phy_rel13_nb_iot.phy[idx].number_of_dl_layers_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].number_of_ul_layers_supported, out.pnf_phy_rel13_nb_iot.phy[idx].number_of_ul_layers_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].maximum_3gpp_release_supported, out.pnf_phy_rel13_nb_iot.phy[idx].maximum_3gpp_release_supported);
		CU_ASSERT_EQUAL(in.pnf_phy_rel13_nb_iot.phy[idx].nmm_modes_supported, out.pnf_phy_rel13_nb_iot.phy[idx].nmm_modes_supported);
	}


}

void nfapi_test_pnf_config_request()
{
	uint16_t idx;
	nfapi_pnf_config_request_t in;
	nfapi_pnf_config_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_CONFIG_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.pnf_phy_rf_config.tl.tag = NFAPI_PNF_PHY_RF_TAG;
	in.pnf_phy_rf_config.number_phy_rf_config_info = 2;
	in.pnf_phy_rf_config.phy_rf_config[0].phy_id = 0;
	in.pnf_phy_rf_config.phy_rf_config[0].phy_config_index = 0;
	in.pnf_phy_rf_config.phy_rf_config[0].rf_config_index = 0;


	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpack_result = nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(unpack_result, 1);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	//CU_ASSERT_EQUAL(in.error_code, out.error_code);

	CU_ASSERT_EQUAL(in.pnf_phy_rf_config.tl.tag, out.pnf_phy_rf_config.tl.tag);
	CU_ASSERT_EQUAL(in.pnf_phy_rf_config.number_phy_rf_config_info, out.pnf_phy_rf_config.number_phy_rf_config_info);

	for(idx = 0; idx < out.pnf_phy_rf_config.number_phy_rf_config_info; ++idx)
	{
		CU_ASSERT_EQUAL(in.pnf_phy_rf_config.phy_rf_config[idx].phy_id, out.pnf_phy_rf_config.phy_rf_config[idx].phy_id);
		CU_ASSERT_EQUAL(in.pnf_phy_rf_config.phy_rf_config[idx].phy_config_index, out.pnf_phy_rf_config.phy_rf_config[idx].phy_config_index);
		CU_ASSERT_EQUAL(in.pnf_phy_rf_config.phy_rf_config[idx].rf_config_index, out.pnf_phy_rf_config.phy_rf_config[idx].rf_config_index);
	}
}



void nfapi_test_pnf_config_response()
{
	nfapi_pnf_config_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_config_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_pnf_config_response1()
{
	uint8_t buffer[NFAPI_HEADER_LENGTH  + 1];

	nfapi_pnf_config_response_t in;
	memset(&in, 0, sizeof(in));
	//nfapi_pnf_config_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), buffer, sizeof(buffer), 0);
	CU_ASSERT_EQUAL(packedMessageLength, -1);

	//nfapi_p5_message_unpack(buffer, sizeof(buffer), &out, sizeof(out), 0);
	//CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	//CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_pnf_start_request()
{
	nfapi_pnf_start_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_start_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_START_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}

void nfapi_test_pnf_start_response()
{
	nfapi_pnf_start_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_start_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_START_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_pnf_stop_request()
{
	nfapi_pnf_stop_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_stop_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_STOP_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}
void nfapi_test_pnf_stop_response()
{
	nfapi_pnf_stop_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_pnf_stop_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PNF_STOP_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}

void nfapi_test_param_request()
{
	nfapi_param_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_param_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PARAM_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}
void nfapi_test_param_response()
{
	#define SET_TLV(_TLV, _TAG, _VALUE) { _TLV.tl.tag = _TAG; _TLV.value = _VALUE; num_tlv++;	}
	#define ASSERT_TLV(IN_TLV, OUT_TLV) { CU_ASSERT_EQUAL(IN_TLV.tl.tag, OUT_TLV.tl.tag); CU_ASSERT_EQUAL(IN_TLV.value, OUT_TLV.value);}

	nfapi_param_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_param_response_t out;
	memset(&out, 0, sizeof(out));

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_PARAM_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	uint16_t idx = 0;
	uint16_t num_tlv = 0; // Bit of a hack, it is incremented in the SET_UINT16_TLV macro

	SET_TLV(in.l1_status.phy_state, NFAPI_L1_STATUS_PHY_STATE_TAG, 1);
	SET_TLV(in.phy_capabilities.dl_bandwidth_support, NFAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG, 63);
	SET_TLV(in.phy_capabilities.ul_bandwidth_support, NFAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG, 63);
	SET_TLV(in.phy_capabilities.dl_modulation_support, NFAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG, 3);
	SET_TLV(in.phy_capabilities.ul_modulation_support, NFAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG, 3);
	SET_TLV(in.phy_capabilities.phy_antenna_capability, NFAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG, 8);
	SET_TLV(in.phy_capabilities.release_capability, NFAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG, 63);
	SET_TLV(in.phy_capabilities.mbsfn_capability, NFAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG, 0);
	
	SET_TLV(in.laa_capability.laa_support, NFAPI_LAA_CAPABILITY_LAA_SUPPORT_TAG, 1);
	SET_TLV(in.laa_capability.pd_sensing_lbt_support, NFAPI_LAA_CAPABILITY_PD_SENSING_LBT_SUPPORT_TAG, 0);
	SET_TLV(in.laa_capability.multi_carrier_lbt_support, NFAPI_LAA_CAPABILITY_MULTI_CARRIER_LBT_SUPPORT_TAG, 3);
	SET_TLV(in.laa_capability.partial_sf_support, NFAPI_LAA_CAPABILITY_PARTIAL_SF_SUPPORT_TAG, 2);

	SET_TLV(in.subframe_config.duplex_mode, NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG, 1);
	SET_TLV(in.subframe_config.pcfich_power_offset, NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG, 1000);
	SET_TLV(in.subframe_config.pb, NFAPI_SUBFRAME_CONFIG_PB_TAG, 1);
	SET_TLV(in.subframe_config.dl_cyclic_prefix_type, NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG, 0);
	SET_TLV(in.subframe_config.ul_cyclic_prefix_type, NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG, 0);

	SET_TLV(in.rf_config.dl_channel_bandwidth, NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG, 50);
	SET_TLV(in.rf_config.ul_channel_bandwidth, NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG, 50);
	SET_TLV(in.rf_config.reference_signal_power, NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG, 120);
	SET_TLV(in.rf_config.tx_antenna_ports, NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG, 2);
	SET_TLV(in.rf_config.rx_antenna_ports, NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG, 2);

	SET_TLV(in.phich_config.phich_resource, NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG, 3);
	SET_TLV(in.phich_config.phich_duration, NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG, 0);
	SET_TLV(in.phich_config.phich_power_offset, NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG, 500);

	SET_TLV(in.sch_config.primary_synchronization_signal_epre_eprers, NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG, 1000);
	SET_TLV(in.sch_config.secondary_synchronization_signal_epre_eprers, NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG, 2000);
	SET_TLV(in.sch_config.physical_cell_id, NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG, 12344);


	SET_TLV(in.prach_config.configuration_index, NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG, 63);
	SET_TLV(in.prach_config.root_sequence_index, NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG, 800);
	SET_TLV(in.prach_config.zero_correlation_zone_configuration, NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG, 14);
	SET_TLV(in.prach_config.high_speed_flag, NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG, 0);
	SET_TLV(in.prach_config.frequency_offset, NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG, 23);

	SET_TLV(in.pusch_config.hopping_mode, NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG, 0);
	SET_TLV(in.pusch_config.hopping_offset, NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG, 55);
	SET_TLV(in.pusch_config.number_of_subbands, NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG, 1);

	SET_TLV(in.pucch_config.delta_pucch_shift, NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG, 2);
	SET_TLV(in.pucch_config.n_cqi_rb, NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG, 88);
	SET_TLV(in.pucch_config.n_an_cs, NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, 3);
	SET_TLV(in.pucch_config.n1_pucch_an, NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG, 2015);

	SET_TLV(in.srs_config.bandwidth_configuration, NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG, 3);
	SET_TLV(in.srs_config.max_up_pts, NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG, 0);
	SET_TLV(in.srs_config.srs_subframe_configuration, NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG, 11);
	SET_TLV(in.srs_config.srs_acknack_srs_simultaneous_transmission, NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG, 1);

	SET_TLV(in.uplink_reference_signal_config.uplink_rs_hopping, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG, 2);
	SET_TLV(in.uplink_reference_signal_config.group_assignment, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG, 22);
	SET_TLV(in.uplink_reference_signal_config.cyclic_shift_1_for_drms, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG, 2);

	SET_TLV(in.tdd_frame_structure_config.subframe_assignment, NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG, 5);
	SET_TLV(in.tdd_frame_structure_config.special_subframe_patterns, NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG, 3);

	SET_TLV(in.l23_config.data_report_mode, NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG, 1);
	SET_TLV(in.l23_config.sfnsf, NFAPI_L23_CONFIG_SFNSF_TAG, 12354);

	in.nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
	num_tlv++;

	in.nfapi_config.p7_vnf_address_ipv6.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG;
	num_tlv++;

	SET_TLV(in.nfapi_config.p7_vnf_port, NFAPI_NFAPI_P7_VNF_PORT_TAG, 1111);
	
	in.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
	num_tlv++;

	in.nfapi_config.p7_pnf_address_ipv6.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG;
	num_tlv++;

	SET_TLV(in.nfapi_config.p7_pnf_port, NFAPI_NFAPI_P7_PNF_PORT_TAG, 9999);
	SET_TLV(in.nfapi_config.dl_ue_per_sf, NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG, 4);
	SET_TLV(in.nfapi_config.ul_ue_per_sf, NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG, 4);

	in.nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
	in.nfapi_config.rf_bands.number_rf_bands = 2;
	in.nfapi_config.rf_bands.rf_band[0] = 783;
	num_tlv++;

	SET_TLV(in.nfapi_config.timing_window, NFAPI_NFAPI_TIMING_WINDOW_TAG, 29);
	SET_TLV(in.nfapi_config.timing_info_mode, NFAPI_NFAPI_TIMING_INFO_MODE_TAG, 2);
	SET_TLV(in.nfapi_config.timing_info_period, NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG, 23);
	SET_TLV(in.nfapi_config.max_transmit_power, NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG, 123);
	SET_TLV(in.nfapi_config.earfcn, NFAPI_NFAPI_EARFCN_TAG, 1800);

	in.nfapi_config.nmm_gsm_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands = 2;
	num_tlv++;

	in.nfapi_config.nmm_umts_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands = 3;
	num_tlv++;

	in.nfapi_config.nmm_lte_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands = 1;
	num_tlv++;

	SET_TLV(in.nfapi_config.nmm_uplink_rssi_supported, NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG, 1);


	in.num_tlv = num_tlv;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
	CU_ASSERT_EQUAL(in.num_tlv, out.num_tlv);

	ASSERT_TLV(in.l1_status.phy_state, out.l1_status.phy_state);
	ASSERT_TLV(in.phy_capabilities.dl_bandwidth_support, out.phy_capabilities.dl_bandwidth_support);
	ASSERT_TLV(in.phy_capabilities.ul_bandwidth_support, out.phy_capabilities.ul_bandwidth_support);
	ASSERT_TLV(in.phy_capabilities.dl_modulation_support, out.phy_capabilities.dl_modulation_support);
	ASSERT_TLV(in.phy_capabilities.ul_modulation_support, out.phy_capabilities.ul_modulation_support);
	ASSERT_TLV(in.phy_capabilities.phy_antenna_capability, out.phy_capabilities.phy_antenna_capability);
	ASSERT_TLV(in.phy_capabilities.release_capability, out.phy_capabilities.release_capability);
	ASSERT_TLV(in.phy_capabilities.mbsfn_capability, out.phy_capabilities.mbsfn_capability);

	ASSERT_TLV(in.laa_capability.laa_support, in.laa_capability.laa_support);
	ASSERT_TLV(in.laa_capability.pd_sensing_lbt_support, in.laa_capability.pd_sensing_lbt_support);
	ASSERT_TLV(in.laa_capability.multi_carrier_lbt_support, in.laa_capability.multi_carrier_lbt_support);
	ASSERT_TLV(in.laa_capability.partial_sf_support, in.laa_capability.partial_sf_support);

	ASSERT_TLV(in.subframe_config.duplex_mode, out.subframe_config.duplex_mode);
	ASSERT_TLV(in.subframe_config.pcfich_power_offset, out.subframe_config.pcfich_power_offset);
	ASSERT_TLV(in.subframe_config.pb, out.subframe_config.pb);
	ASSERT_TLV(in.subframe_config.dl_cyclic_prefix_type, out.subframe_config.dl_cyclic_prefix_type);
	ASSERT_TLV(in.subframe_config.ul_cyclic_prefix_type, out.subframe_config.ul_cyclic_prefix_type);

	ASSERT_TLV(in.rf_config.dl_channel_bandwidth, out.rf_config.dl_channel_bandwidth);
	ASSERT_TLV(in.rf_config.ul_channel_bandwidth, out.rf_config.ul_channel_bandwidth);
	ASSERT_TLV(in.rf_config.reference_signal_power, out.rf_config.reference_signal_power);
	ASSERT_TLV(in.rf_config.tx_antenna_ports, out.rf_config.tx_antenna_ports);
	ASSERT_TLV(in.rf_config.rx_antenna_ports, out.rf_config.rx_antenna_ports);

	ASSERT_TLV(in.phich_config.phich_resource, out.phich_config.phich_resource);
	ASSERT_TLV(in.phich_config.phich_duration, out.phich_config.phich_duration);
	ASSERT_TLV(in.phich_config.phich_power_offset, out.phich_config.phich_power_offset);

	ASSERT_TLV(in.sch_config.primary_synchronization_signal_epre_eprers, out.sch_config.primary_synchronization_signal_epre_eprers);
	ASSERT_TLV(in.sch_config.secondary_synchronization_signal_epre_eprers, out.sch_config.secondary_synchronization_signal_epre_eprers);
	ASSERT_TLV(in.sch_config.physical_cell_id, out.sch_config.physical_cell_id);


	ASSERT_TLV(in.prach_config.configuration_index, out.prach_config.configuration_index);
	ASSERT_TLV(in.prach_config.root_sequence_index, out.prach_config.root_sequence_index);
	ASSERT_TLV(in.prach_config.zero_correlation_zone_configuration, out.prach_config.zero_correlation_zone_configuration);
	ASSERT_TLV(in.prach_config.high_speed_flag, out.prach_config.high_speed_flag);
	ASSERT_TLV(in.prach_config.frequency_offset, out.prach_config.frequency_offset);
	
	ASSERT_TLV(in.pusch_config.hopping_mode, in.pusch_config.hopping_mode);
	ASSERT_TLV(in.pusch_config.hopping_offset, in.pusch_config.hopping_offset);
	ASSERT_TLV(in.pusch_config.number_of_subbands, in.pusch_config.number_of_subbands);

	ASSERT_TLV(in.pucch_config.delta_pucch_shift, out.pucch_config.delta_pucch_shift);
	ASSERT_TLV(in.pucch_config.n_cqi_rb, out.pucch_config.n_cqi_rb);
	ASSERT_TLV(in.pucch_config.n_an_cs, out.pucch_config.n_an_cs);
	ASSERT_TLV(in.pucch_config.n1_pucch_an, out.pucch_config.n1_pucch_an);

	ASSERT_TLV(in.srs_config.bandwidth_configuration, out.srs_config.bandwidth_configuration);
	ASSERT_TLV(in.srs_config.max_up_pts, out.srs_config.max_up_pts);
	ASSERT_TLV(in.srs_config.srs_subframe_configuration, out.srs_config.srs_subframe_configuration);
	ASSERT_TLV(in.srs_config.srs_acknack_srs_simultaneous_transmission, out.srs_config.srs_acknack_srs_simultaneous_transmission);

	ASSERT_TLV(in.uplink_reference_signal_config.uplink_rs_hopping, out.uplink_reference_signal_config.uplink_rs_hopping);
	ASSERT_TLV(in.uplink_reference_signal_config.group_assignment, out.uplink_reference_signal_config.group_assignment);
	ASSERT_TLV(in.uplink_reference_signal_config.cyclic_shift_1_for_drms, out.uplink_reference_signal_config.cyclic_shift_1_for_drms);

	ASSERT_TLV(in.tdd_frame_structure_config.subframe_assignment, out.tdd_frame_structure_config.subframe_assignment);
	ASSERT_TLV(in.tdd_frame_structure_config.special_subframe_patterns, out.tdd_frame_structure_config.special_subframe_patterns);

	ASSERT_TLV(in.l23_config.data_report_mode, out.l23_config.data_report_mode);
	ASSERT_TLV(in.l23_config.sfnsf, out.l23_config.sfnsf);

	CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv4.tl.tag, out.nfapi_config.p7_vnf_address_ipv4.tl.tag);
	for(idx = 0; idx < NFAPI_IPV4_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv4.address[idx], out.nfapi_config.p7_vnf_address_ipv4.address[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv6.tl.tag, out.nfapi_config.p7_vnf_address_ipv6.tl.tag);
	for(idx = 0; idx < NFAPI_IPV6_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv6.address[idx], out.nfapi_config.p7_vnf_address_ipv6.address[idx]);
	}

	ASSERT_TLV(in.nfapi_config.p7_vnf_port, out.nfapi_config.p7_vnf_port);
	
	CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv4.tl.tag, out.nfapi_config.p7_pnf_address_ipv4.tl.tag);
	for(idx = 0; idx < NFAPI_IPV4_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv4.address[idx], out.nfapi_config.p7_pnf_address_ipv4.address[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv6.tl.tag, out.nfapi_config.p7_pnf_address_ipv6.tl.tag);
	for(idx = 0; idx < NFAPI_IPV6_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv6.address[idx], out.nfapi_config.p7_pnf_address_ipv6.address[idx]);
	}

	ASSERT_TLV(in.nfapi_config.p7_pnf_port, out.nfapi_config.p7_pnf_port);
	ASSERT_TLV(in.nfapi_config.dl_ue_per_sf, out.nfapi_config.dl_ue_per_sf);
	ASSERT_TLV(in.nfapi_config.ul_ue_per_sf, out.nfapi_config.ul_ue_per_sf);

	CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.tl.tag, out.nfapi_config.rf_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.number_rf_bands, out.nfapi_config.rf_bands.number_rf_bands);
	for(idx = 0; idx < out.nfapi_config.rf_bands.number_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.rf_band[idx], out.nfapi_config.rf_bands.rf_band[idx]);
	}

	ASSERT_TLV(in.nfapi_config.timing_window, out.nfapi_config.timing_window);
	ASSERT_TLV(in.nfapi_config.timing_info_mode, out.nfapi_config.timing_info_mode);
	ASSERT_TLV(in.nfapi_config.timing_info_period, out.nfapi_config.timing_info_period);
	ASSERT_TLV(in.nfapi_config.max_transmit_power, out.nfapi_config.max_transmit_power);
	ASSERT_TLV(in.nfapi_config.earfcn, out.nfapi_config.earfcn); 

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.tl.tag, out.nfapi_config.nmm_gsm_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.bands[idx], out.nfapi_config.nmm_gsm_frequency_bands.bands[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.tl.tag, out.nfapi_config.nmm_umts_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.bands[idx], out.nfapi_config.nmm_umts_frequency_bands.bands[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.tl.tag, out.nfapi_config.nmm_lte_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.bands[idx], out.nfapi_config.nmm_lte_frequency_bands.bands[idx]);
	}

	ASSERT_TLV(in.nfapi_config.nmm_uplink_rssi_supported, out.nfapi_config.nmm_uplink_rssi_supported); 
}
void nfapi_test_config_request()
{
	#define SET_TLV(_TLV, _TAG, _VALUE) { _TLV.tl.tag = _TAG; _TLV.value = _VALUE; num_tlv++;	}
	#define ASSERT_TLV(IN_TLV, OUT_TLV) { CU_ASSERT_EQUAL(IN_TLV.tl.tag, OUT_TLV.tl.tag); CU_ASSERT_EQUAL(IN_TLV.value, OUT_TLV.value);}

	nfapi_config_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_config_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CONFIG_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;


	uint16_t idx = 0;
	uint16_t num_tlv = 0; // Bit of a hack, it is incremented in the SET_UINT16_TLV macro

	SET_TLV(in.subframe_config.duplex_mode, NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG, 1);
	SET_TLV(in.subframe_config.pcfich_power_offset, NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG, 1000);
	SET_TLV(in.subframe_config.pb, NFAPI_SUBFRAME_CONFIG_PB_TAG, 1);
	SET_TLV(in.subframe_config.dl_cyclic_prefix_type, NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG, 0);
	SET_TLV(in.subframe_config.ul_cyclic_prefix_type, NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG, 0);

	SET_TLV(in.rf_config.dl_channel_bandwidth, NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG, 50);
	SET_TLV(in.rf_config.ul_channel_bandwidth, NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG, 50);
	SET_TLV(in.rf_config.reference_signal_power, NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG, 120);
	SET_TLV(in.rf_config.tx_antenna_ports, NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG, 2);
	SET_TLV(in.rf_config.rx_antenna_ports, NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG, 2);

	SET_TLV(in.phich_config.phich_resource, NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG, 3);
	SET_TLV(in.phich_config.phich_duration, NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG, 0);
	SET_TLV(in.phich_config.phich_power_offset, NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG, 500);

	SET_TLV(in.sch_config.primary_synchronization_signal_epre_eprers, NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG, 1000);
	SET_TLV(in.sch_config.secondary_synchronization_signal_epre_eprers, NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG, 2000);
	SET_TLV(in.sch_config.physical_cell_id, NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG, 12344);


	SET_TLV(in.prach_config.configuration_index, NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG, 63);
	SET_TLV(in.prach_config.root_sequence_index, NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG, 800);
	SET_TLV(in.prach_config.zero_correlation_zone_configuration, NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG, 14);
	SET_TLV(in.prach_config.high_speed_flag, NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG, 0);
	SET_TLV(in.prach_config.frequency_offset, NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG, 23);

	SET_TLV(in.pusch_config.hopping_mode, NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG, 0);
	SET_TLV(in.pusch_config.hopping_offset, NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG, 55);
	SET_TLV(in.pusch_config.number_of_subbands, NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG, 1);

	SET_TLV(in.pucch_config.delta_pucch_shift, NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG, 2);
	SET_TLV(in.pucch_config.n_cqi_rb, NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG, 88);
	SET_TLV(in.pucch_config.n_an_cs, NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, 3);
	SET_TLV(in.pucch_config.n1_pucch_an, NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG, 2015);

	SET_TLV(in.srs_config.bandwidth_configuration, NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG, 3);
	SET_TLV(in.srs_config.max_up_pts, NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG, 0);
	SET_TLV(in.srs_config.srs_subframe_configuration, NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG, 11);
	SET_TLV(in.srs_config.srs_acknack_srs_simultaneous_transmission, NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG, 1);

	SET_TLV(in.uplink_reference_signal_config.uplink_rs_hopping, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG, 2);
	SET_TLV(in.uplink_reference_signal_config.group_assignment, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG, 22);
	SET_TLV(in.uplink_reference_signal_config.cyclic_shift_1_for_drms, NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG, 2);


	// laa config
	SET_TLV(in.laa_config.ed_threshold_lbt_pdsch, NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG, 45);
	SET_TLV(in.laa_config.ed_threshold_lbt_drs, NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG, 69);
	SET_TLV(in.laa_config.pd_threshold, NFAPI_LAA_CONFIG_PD_THRESHOLD_TAG, 65535);
	SET_TLV(in.laa_config.multi_carrier_type, NFAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG, 3);
	SET_TLV(in.laa_config.multi_carrier_tx, NFAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG, 1);
	SET_TLV(in.laa_config.multi_carrier_freeze, NFAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG, 0);
	SET_TLV(in.laa_config.tx_antenna_ports_drs, NFAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG, 4);
	SET_TLV(in.laa_config.tx_power_drs, NFAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG, 10000);
	//
	// emtc config
	SET_TLV(in.emtc_config.pbch_repetitions_enable_r13, NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG, 1);
	SET_TLV(in.emtc_config.prach_catm_root_sequence_index, NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG, 837); 
	SET_TLV(in.emtc_config.prach_catm_zero_correlation_zone_configuration, NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG, 15); 
	SET_TLV(in.emtc_config.prach_catm_high_speed_flag, NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG, 15); 
	SET_TLV(in.emtc_config.prach_ce_level_0_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG, 1); 
	SET_TLV(in.emtc_config.prach_ce_level_0_configuration_index, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG, 60);
	SET_TLV(in.emtc_config.prach_ce_level_0_frequency_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG, 90);
	SET_TLV(in.emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG, 128);
	SET_TLV(in.emtc_config.prach_ce_level_0_starting_subframe_periodicity, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG, 256);
	SET_TLV(in.emtc_config.prach_ce_level_0_hopping_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG, 1);
	SET_TLV(in.emtc_config.prach_ce_level_0_hopping_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG, 0);
	SET_TLV(in.emtc_config.prach_ce_level_1_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG, 1);
	SET_TLV(in.emtc_config.prach_ce_level_1_configuration_index, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG, 45);
	SET_TLV(in.emtc_config.prach_ce_level_1_frequency_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG, 88);
	SET_TLV(in.emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG, 64);
	SET_TLV(in.emtc_config.prach_ce_level_1_starting_subframe_periodicity, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG, 0xFFFF);
	SET_TLV(in.emtc_config.prach_ce_level_1_hopping_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG, 1);
	SET_TLV(in.emtc_config.prach_ce_level_1_hopping_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG, 22);
	SET_TLV(in.emtc_config.prach_ce_level_2_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG, 1);
	SET_TLV(in.emtc_config.prach_ce_level_2_configuration_index, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG, 63);
	SET_TLV(in.emtc_config.prach_ce_level_2_frequency_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG, 22);
	SET_TLV(in.emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG, 8);
	SET_TLV(in.emtc_config.prach_ce_level_2_starting_subframe_periodicity, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG, 64)
	SET_TLV(in.emtc_config.prach_ce_level_2_hopping_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG, 1);
	SET_TLV(in.emtc_config.prach_ce_level_2_hopping_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG, 45);
	SET_TLV(in.emtc_config.prach_ce_level_3_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG, 0);
	SET_TLV(in.emtc_config.prach_ce_level_3_configuration_index, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG, 45);
	SET_TLV(in.emtc_config.prach_ce_level_3_frequency_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG, 22);
	SET_TLV(in.emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG, 4); 
	SET_TLV(in.emtc_config.prach_ce_level_3_starting_subframe_periodicity, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG, 4);
	SET_TLV(in.emtc_config.prach_ce_level_3_hopping_enable, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG, 0); 
	SET_TLV(in.emtc_config.prach_ce_level_3_hopping_offset, NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG, 0);
	SET_TLV(in.emtc_config.pucch_interval_ulhoppingconfigcommonmodea, NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG, 8);
	SET_TLV(in.emtc_config.pucch_interval_ulhoppingconfigcommonmodeb, NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG, 8);


	SET_TLV(in.tdd_frame_structure_config.subframe_assignment, NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG, 5);
	SET_TLV(in.tdd_frame_structure_config.special_subframe_patterns, NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG, 3);

	SET_TLV(in.l23_config.data_report_mode, NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG, 1);
	SET_TLV(in.l23_config.sfnsf, NFAPI_L23_CONFIG_SFNSF_TAG, 12354);

	in.nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
	num_tlv++;

	in.nfapi_config.p7_vnf_address_ipv6.tl.tag = NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG;
	num_tlv++;

	SET_TLV(in.nfapi_config.p7_vnf_port, NFAPI_NFAPI_P7_VNF_PORT_TAG, 1111);
	
	in.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
	num_tlv++;

	in.nfapi_config.p7_pnf_address_ipv6.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG;
	for(idx = 0; idx < NFAPI_IPV6_ADDRESS_LENGTH; ++idx)
	{
		in.nfapi_config.p7_pnf_address_ipv6.address[idx] = idx;
	}
	num_tlv++;

	SET_TLV(in.nfapi_config.p7_pnf_port, NFAPI_NFAPI_P7_PNF_PORT_TAG, 9999);
	SET_TLV(in.nfapi_config.dl_ue_per_sf, NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG, 4);
	SET_TLV(in.nfapi_config.ul_ue_per_sf, NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG, 4);

	in.nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
	in.nfapi_config.rf_bands.number_rf_bands = 2;
	in.nfapi_config.rf_bands.rf_band[0] = 783;
	num_tlv++;

	SET_TLV(in.nfapi_config.timing_window, NFAPI_NFAPI_TIMING_WINDOW_TAG, 29);
	SET_TLV(in.nfapi_config.timing_info_mode, NFAPI_NFAPI_TIMING_INFO_MODE_TAG, 2);
	SET_TLV(in.nfapi_config.timing_info_period, NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG, 23);
	SET_TLV(in.nfapi_config.max_transmit_power, NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG, 123);
	SET_TLV(in.nfapi_config.earfcn, NFAPI_NFAPI_EARFCN_TAG, 1800);

	in.nfapi_config.nmm_gsm_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands = 2;
	num_tlv++;

	in.nfapi_config.nmm_umts_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands = 3;
	num_tlv++;

	in.nfapi_config.nmm_lte_frequency_bands.tl.tag = NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG;
	in.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands = 1;
	num_tlv++;

	SET_TLV(in.nfapi_config.nmm_uplink_rssi_supported, NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG, 1);


	in.num_tlv = num_tlv;

	in.vendor_extension = 0;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.num_tlv, out.num_tlv);

	ASSERT_TLV(in.subframe_config.duplex_mode, out.subframe_config.duplex_mode);
	ASSERT_TLV(in.subframe_config.pcfich_power_offset, out.subframe_config.pcfich_power_offset);
	ASSERT_TLV(in.subframe_config.pb, out.subframe_config.pb);
	ASSERT_TLV(in.subframe_config.dl_cyclic_prefix_type, out.subframe_config.dl_cyclic_prefix_type);
	ASSERT_TLV(in.subframe_config.ul_cyclic_prefix_type, out.subframe_config.ul_cyclic_prefix_type);

	ASSERT_TLV(in.rf_config.dl_channel_bandwidth, out.rf_config.dl_channel_bandwidth);
	ASSERT_TLV(in.rf_config.ul_channel_bandwidth, out.rf_config.ul_channel_bandwidth);
	ASSERT_TLV(in.rf_config.reference_signal_power, out.rf_config.reference_signal_power);
	ASSERT_TLV(in.rf_config.tx_antenna_ports, out.rf_config.tx_antenna_ports);
	ASSERT_TLV(in.rf_config.rx_antenna_ports, out.rf_config.rx_antenna_ports);

	ASSERT_TLV(in.phich_config.phich_resource, out.phich_config.phich_resource);
	ASSERT_TLV(in.phich_config.phich_duration, out.phich_config.phich_duration);
	ASSERT_TLV(in.phich_config.phich_power_offset, out.phich_config.phich_power_offset);

	ASSERT_TLV(in.sch_config.primary_synchronization_signal_epre_eprers, out.sch_config.primary_synchronization_signal_epre_eprers);
	ASSERT_TLV(in.sch_config.secondary_synchronization_signal_epre_eprers, out.sch_config.secondary_synchronization_signal_epre_eprers);
	ASSERT_TLV(in.sch_config.physical_cell_id, out.sch_config.physical_cell_id);


	ASSERT_TLV(in.prach_config.configuration_index, out.prach_config.configuration_index);
	ASSERT_TLV(in.prach_config.root_sequence_index, out.prach_config.root_sequence_index);
	ASSERT_TLV(in.prach_config.zero_correlation_zone_configuration, out.prach_config.zero_correlation_zone_configuration);
	ASSERT_TLV(in.prach_config.high_speed_flag, out.prach_config.high_speed_flag);
	ASSERT_TLV(in.prach_config.frequency_offset, out.prach_config.frequency_offset);
	
	ASSERT_TLV(in.pusch_config.hopping_mode, in.pusch_config.hopping_mode);
	ASSERT_TLV(in.pusch_config.hopping_offset, in.pusch_config.hopping_offset);
	ASSERT_TLV(in.pusch_config.number_of_subbands, in.pusch_config.number_of_subbands);

	ASSERT_TLV(in.pucch_config.delta_pucch_shift, out.pucch_config.delta_pucch_shift);
	ASSERT_TLV(in.pucch_config.n_cqi_rb, out.pucch_config.n_cqi_rb);
	ASSERT_TLV(in.pucch_config.n_an_cs, out.pucch_config.n_an_cs);
	ASSERT_TLV(in.pucch_config.n1_pucch_an, out.pucch_config.n1_pucch_an);

	ASSERT_TLV(in.srs_config.bandwidth_configuration, out.srs_config.bandwidth_configuration);
	ASSERT_TLV(in.srs_config.max_up_pts, out.srs_config.max_up_pts);
	ASSERT_TLV(in.srs_config.srs_subframe_configuration, out.srs_config.srs_subframe_configuration);
	ASSERT_TLV(in.srs_config.srs_acknack_srs_simultaneous_transmission, out.srs_config.srs_acknack_srs_simultaneous_transmission);

	ASSERT_TLV(in.uplink_reference_signal_config.uplink_rs_hopping, out.uplink_reference_signal_config.uplink_rs_hopping);
	ASSERT_TLV(in.uplink_reference_signal_config.group_assignment, out.uplink_reference_signal_config.group_assignment);
	ASSERT_TLV(in.uplink_reference_signal_config.cyclic_shift_1_for_drms, out.uplink_reference_signal_config.cyclic_shift_1_for_drms);

	ASSERT_TLV(in.tdd_frame_structure_config.subframe_assignment, out.tdd_frame_structure_config.subframe_assignment);
	ASSERT_TLV(in.tdd_frame_structure_config.special_subframe_patterns, out.tdd_frame_structure_config.special_subframe_patterns);

	// laa config
	ASSERT_TLV(in.laa_config.ed_threshold_lbt_pdsch, out.laa_config.ed_threshold_lbt_pdsch);
	ASSERT_TLV(in.laa_config.ed_threshold_lbt_drs, out.laa_config.ed_threshold_lbt_drs);
	ASSERT_TLV(in.laa_config.pd_threshold, out.laa_config.pd_threshold);
	ASSERT_TLV(in.laa_config.multi_carrier_type, out.laa_config.multi_carrier_type);
	ASSERT_TLV(in.laa_config.multi_carrier_tx, out.laa_config.multi_carrier_tx);
	ASSERT_TLV(in.laa_config.multi_carrier_freeze, out.laa_config.multi_carrier_freeze);
	ASSERT_TLV(in.laa_config.tx_antenna_ports_drs, out.laa_config.tx_antenna_ports_drs);
	ASSERT_TLV(in.laa_config.tx_power_drs, out.laa_config.tx_power_drs);
	// emtc config
	ASSERT_TLV(in.emtc_config.pbch_repetitions_enable_r13, out.emtc_config.pbch_repetitions_enable_r13);
	ASSERT_TLV(in.emtc_config.prach_catm_root_sequence_index, out.emtc_config.prach_catm_root_sequence_index);
	ASSERT_TLV(in.emtc_config.prach_catm_zero_correlation_zone_configuration, out.emtc_config.prach_catm_zero_correlation_zone_configuration);
	ASSERT_TLV(in.emtc_config.prach_catm_high_speed_flag, out.emtc_config.prach_catm_high_speed_flag);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_enable, out.emtc_config.prach_ce_level_0_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_configuration_index, out.emtc_config.prach_ce_level_0_configuration_index);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_frequency_offset, out.emtc_config.prach_ce_level_0_frequency_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt, out.emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_starting_subframe_periodicity, out.emtc_config.prach_ce_level_0_starting_subframe_periodicity);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_hopping_enable, out.emtc_config.prach_ce_level_0_hopping_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_0_hopping_offset, out.emtc_config.prach_ce_level_0_hopping_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_enable, out.emtc_config.prach_ce_level_1_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_configuration_index, out.emtc_config.prach_ce_level_1_configuration_index);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_frequency_offset, out.emtc_config.prach_ce_level_1_frequency_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt, out.emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_starting_subframe_periodicity, out.emtc_config.prach_ce_level_1_starting_subframe_periodicity);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_hopping_enable, out.emtc_config.prach_ce_level_1_hopping_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_1_hopping_offset, out.emtc_config.prach_ce_level_1_hopping_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_enable, out.emtc_config.prach_ce_level_2_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_configuration_index, out.emtc_config.prach_ce_level_2_configuration_index);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_frequency_offset, out.emtc_config.prach_ce_level_2_frequency_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt, out.emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_starting_subframe_periodicity, out.emtc_config.prach_ce_level_2_starting_subframe_periodicity);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_hopping_enable, out.emtc_config.prach_ce_level_2_hopping_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_2_hopping_offset, out.emtc_config.prach_ce_level_2_hopping_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_enable, out.emtc_config.prach_ce_level_3_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_configuration_index, out.emtc_config.prach_ce_level_3_configuration_index);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_frequency_offset, out.emtc_config.prach_ce_level_3_frequency_offset);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt, out.emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_starting_subframe_periodicity, out.emtc_config.prach_ce_level_3_starting_subframe_periodicity);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_hopping_enable, out.emtc_config.prach_ce_level_3_hopping_enable);
	ASSERT_TLV(in.emtc_config.prach_ce_level_3_hopping_offset, out.emtc_config.prach_ce_level_3_hopping_offset);
	ASSERT_TLV(in.emtc_config.pucch_interval_ulhoppingconfigcommonmodea, out.emtc_config.pucch_interval_ulhoppingconfigcommonmodea);
	ASSERT_TLV(in.emtc_config.pucch_interval_ulhoppingconfigcommonmodeb, out.emtc_config.pucch_interval_ulhoppingconfigcommonmodeb);

	ASSERT_TLV(in.l23_config.data_report_mode, out.l23_config.data_report_mode);
	ASSERT_TLV(in.l23_config.sfnsf, out.l23_config.sfnsf);

	CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv4.tl.tag, out.nfapi_config.p7_vnf_address_ipv4.tl.tag);
	for(idx = 0; idx < NFAPI_IPV4_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv4.address[idx], out.nfapi_config.p7_vnf_address_ipv4.address[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv6.tl.tag, out.nfapi_config.p7_vnf_address_ipv6.tl.tag);
	for(idx = 0; idx < NFAPI_IPV6_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_vnf_address_ipv6.address[idx], out.nfapi_config.p7_vnf_address_ipv6.address[idx]);
	}

	ASSERT_TLV(in.nfapi_config.p7_vnf_port, out.nfapi_config.p7_vnf_port);
	
	CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv4.tl.tag, out.nfapi_config.p7_pnf_address_ipv4.tl.tag);
	for(idx = 0; idx < NFAPI_IPV4_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv4.address[idx], out.nfapi_config.p7_pnf_address_ipv4.address[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv6.tl.tag, out.nfapi_config.p7_pnf_address_ipv6.tl.tag);
	for(idx = 0; idx < NFAPI_IPV6_ADDRESS_LENGTH; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.p7_pnf_address_ipv6.address[idx], out.nfapi_config.p7_pnf_address_ipv6.address[idx]);
	}

	ASSERT_TLV(in.nfapi_config.p7_pnf_port, out.nfapi_config.p7_pnf_port);
	ASSERT_TLV(in.nfapi_config.dl_ue_per_sf, out.nfapi_config.dl_ue_per_sf);
	ASSERT_TLV(in.nfapi_config.ul_ue_per_sf, out.nfapi_config.ul_ue_per_sf);

	CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.tl.tag, out.nfapi_config.rf_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.number_rf_bands, out.nfapi_config.rf_bands.number_rf_bands);
	for(idx = 0; idx < out.nfapi_config.rf_bands.number_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.rf_bands.rf_band[idx], out.nfapi_config.rf_bands.rf_band[idx]);
	}

	ASSERT_TLV(in.nfapi_config.timing_window, out.nfapi_config.timing_window);
	ASSERT_TLV(in.nfapi_config.timing_info_mode, out.nfapi_config.timing_info_mode);
	ASSERT_TLV(in.nfapi_config.timing_info_period, out.nfapi_config.timing_info_period);
	ASSERT_TLV(in.nfapi_config.max_transmit_power, out.nfapi_config.max_transmit_power);
	ASSERT_TLV(in.nfapi_config.earfcn, out.nfapi_config.earfcn); 

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.tl.tag, out.nfapi_config.nmm_gsm_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_gsm_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_gsm_frequency_bands.bands[idx], out.nfapi_config.nmm_gsm_frequency_bands.bands[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.tl.tag, out.nfapi_config.nmm_umts_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_umts_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_umts_frequency_bands.bands[idx], out.nfapi_config.nmm_umts_frequency_bands.bands[idx]);
	}

	CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.tl.tag, out.nfapi_config.nmm_lte_frequency_bands.tl.tag);
	CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands, out.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands);
	for(idx = 0; idx < out.nfapi_config.nmm_lte_frequency_bands.number_of_rf_bands; ++idx)
	{
		CU_ASSERT_EQUAL(in.nfapi_config.nmm_lte_frequency_bands.bands[idx], out.nfapi_config.nmm_lte_frequency_bands.bands[idx]);
	}

	ASSERT_TLV(in.nfapi_config.nmm_uplink_rssi_supported, out.nfapi_config.nmm_uplink_rssi_supported); 
}
void nfapi_test_config_response()
{
	nfapi_config_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_config_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CONFIG_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_start_request()
{
	nfapi_start_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_start_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_START_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;


	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
}
void nfapi_test_start_response()
{
	nfapi_start_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_start_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_START_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_stop_request()
{
	nfapi_stop_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_stop_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_STOP_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;


	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

}
void nfapi_test_stop_response()
{
	nfapi_stop_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_stop_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_STOP_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);
}
void nfapi_test_measurement_request()
{
//	#define SET_TLV(_TLV, _TAG, _VALUE) { _TLV.tl.tag = _TAG; _TLV.value = _VALUE; }
//	#define ASSERT_TLV(IN_TLV, OUT_TLV) { CU_ASSERT_EQUAL(IN_TLV.tl.tag, OUT_TLV.tl.tag); CU_ASSERT_EQUAL(IN_TLV.value, OUT_TLV.value);}
	uint16_t num_tlv = 0;
	nfapi_measurement_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_measurement_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_MEASUREMENT_REQUEST;
	in.header.message_length = 0;
	in.header.spare = 0;

	SET_TLV(in.dl_rs_tx_power, NFAPI_MEASUREMENT_REQUEST_DL_RS_XTX_POWER_TAG, 1);
	SET_TLV(in.received_interference_power, NFAPI_MEASUREMENT_REQUEST_RECEIVED_INTERFERENCE_POWER_TAG, 123);
	SET_TLV(in.thermal_noise_power, NFAPI_MEASUREMENT_REQUEST_THERMAL_NOISE_POWER_TAG, 255);

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);


	ASSERT_TLV(in.dl_rs_tx_power, out.dl_rs_tx_power);
	ASSERT_TLV(in.received_interference_power, out.received_interference_power);
	ASSERT_TLV(in.thermal_noise_power, out.thermal_noise_power);
}
void nfapi_test_measurement_response()
{
	uint16_t num_tlv = 0;
	nfapi_measurement_response_t in;
	memset(&in, 0, sizeof(in));
	nfapi_measurement_response_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
	in.header.message_length = 0;
	in.header.spare = 0;

	in.error_code = NFAPI_MSG_OK;

	SET_TLV(in.dl_rs_tx_power_measurement, NFAPI_MEASUREMENT_RESPONSE_DL_RS_POWER_MEASUREMENT_TAG, -400);

	in.received_interference_power_measurement.tl.tag = NFAPI_MEASUREMENT_RESPONSE_RECEIVED_INTERFERENCE_POWER_MEASUREMENT_TAG;
	in.received_interference_power_measurement.number_of_resource_blocks = 100;
	in.received_interference_power_measurement.received_interference_power[0] = -8767;

	SET_TLV(in.thermal_noise_power_measurement, NFAPI_MEASUREMENT_RESPONSE_THERMAL_NOISE_MEASUREMENT_TAG, -8900);

	int packedMessageLength = nfapi_p5_message_pack(&in, sizeof(in), gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p5_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.error_code, out.error_code);

	ASSERT_TLV(in.dl_rs_tx_power_measurement, out.dl_rs_tx_power_measurement);
	CU_ASSERT_EQUAL(in.received_interference_power_measurement.tl.tag, out.received_interference_power_measurement.tl.tag);
	CU_ASSERT_EQUAL(in.received_interference_power_measurement.number_of_resource_blocks, out.received_interference_power_measurement.number_of_resource_blocks);
	// check array
	ASSERT_TLV(in.thermal_noise_power_measurement, out.thermal_noise_power_measurement);
}

void nfapi_test_dl_config_request()
{


	uint16_t idx = 0;
	nfapi_dl_config_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_dl_config_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_DL_CONFIG_REQUEST;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 0x3FFF;

	in.dl_config_request_body.tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
	in.dl_config_request_body.number_pdcch_ofdm_symbols = 4;
	in.dl_config_request_body.number_dci = 255;
	in.dl_config_request_body.number_pdu = 0;
	in.dl_config_request_body.number_pdsch_rnti = 0;
	in.dl_config_request_body.transmission_power_pcfich = 10000;

	in.dl_config_request_body.dl_config_pdu_list = (nfapi_dl_config_request_pdu_t*)(malloc(sizeof(nfapi_dl_config_request_pdu_t) * 12));
	
	in.dl_config_request_body.dl_config_pdu_list[0].pdu_type = 0;
	in.dl_config_request_body.dl_config_pdu_list[0].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL9_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL10_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL11_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL12_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.primary_cell_type = rand8(0, 2);
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_flag = rand8(0, 1);
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.number_ul_dl_configurations = rand8(0, 5);
	for(idx = 0; idx <in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.number_ul_dl_configurations; ++idx)
	{
		in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_indication[idx] = rand8(1, 5);
	}
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL13_TAG;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm_struct_flag = 1; //rand8(0, 1);
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_prb_per_subband = 3;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.number_of_subbands = 2;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_antennas = 2;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].subband_index = 1;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].scheduled_ues = 2;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].precoding_value[0][0] = 0x321;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].precoding_value[0][1] = 0x321;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].precoding_value[1][0] = 0x321;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[0].precoding_value[1][1] = 0x321;
	
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[1].subband_index = 2;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[1].scheduled_ues = 1;	
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[1].precoding_value[0][0] = 0x32A;
	in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[1].precoding_value[1][0] = 0x32A;

	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[1].pdu_type = 1;
	in.dl_config_request_body.dl_config_pdu_list[1].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[1].bch_pdu.bch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[2].pdu_type = 2;
	in.dl_config_request_body.dl_config_pdu_list[2].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[2].mch_pdu.mch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_MCH_PDU_REL8_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[3].pdu_type = 3;
	in.dl_config_request_body.dl_config_pdu_list[3].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL9_TAG;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL11_TAG;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL12_TAG;
	in.dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[4].pdu_type = 4;
	in.dl_config_request_body.dl_config_pdu_list[4].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[4].pch_pdu.pch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL8_TAG;
	in.dl_config_request_body.dl_config_pdu_list[4].pch_pdu.pch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[5].pdu_type = 5;
	in.dl_config_request_body.dl_config_pdu_list[5].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[5].prs_pdu.prs_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_PRS_PDU_REL9_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[6].pdu_type = 6;
	in.dl_config_request_body.dl_config_pdu_list[6].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[6].csi_rs_pdu.csi_rs_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL10_TAG;
	in.dl_config_request_body.dl_config_pdu_list[6].csi_rs_pdu.csi_rs_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[7].pdu_type = 7;
	in.dl_config_request_body.dl_config_pdu_list[7].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL8_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL9_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL10_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL11_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL12_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL13_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_params_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL11_TAG;
	in.dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_params_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL13_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[8].pdu_type = 8;
	in.dl_config_request_body.dl_config_pdu_list[8].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[8].mpdcch_pdu.mpdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_MPDCCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;

	in.dl_config_request_body.dl_config_pdu_list[9].pdu_type = 9;
	in.dl_config_request_body.dl_config_pdu_list[9].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NBCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;
	
	in.dl_config_request_body.dl_config_pdu_list[10].pdu_type = 10;
	in.dl_config_request_body.dl_config_pdu_list[10].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NPDCCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;
	
	in.dl_config_request_body.dl_config_pdu_list[11].pdu_type = 11;
	in.dl_config_request_body.dl_config_pdu_list[11].pdu_size = 123;
	in.dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NDLSCH_PDU_REL13_TAG;
	in.dl_config_request_body.number_pdu++;
	

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, 0);
	CU_ASSERT_NOT_EQUAL(packedMessageLength, -1);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	CU_ASSERT_EQUAL(in.dl_config_request_body.tl.tag, out.dl_config_request_body.tl.tag);
	CU_ASSERT_EQUAL(in.dl_config_request_body.number_pdcch_ofdm_symbols, out.dl_config_request_body.number_pdcch_ofdm_symbols);
	CU_ASSERT_EQUAL(in.dl_config_request_body.number_dci, out.dl_config_request_body.number_dci);
	CU_ASSERT_EQUAL(in.dl_config_request_body.number_pdu, out.dl_config_request_body.number_pdu);
	CU_ASSERT_EQUAL(in.dl_config_request_body.number_pdsch_rnti, out.dl_config_request_body.number_pdsch_rnti);
	CU_ASSERT_EQUAL(in.dl_config_request_body.transmission_power_pcfich, out.dl_config_request_body.transmission_power_pcfich);


	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].pdu_type, out.dl_config_request_body.dl_config_pdu_list[0].pdu_type);
	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].pdu_size, out.dl_config_request_body.dl_config_pdu_list[0].pdu_size);
	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.dci_format);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.rnti);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.mcs_1);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.transport_block_to_codeword_swap_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.mcs_2);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_2);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.harq_process);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.tpmi);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.pmi);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.precoding_information);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.tpc);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.ngap);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.transport_block_size_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.downlink_power_offset);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.allocate_prach_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.preamble_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.prach_mask_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel8.transmission_power);

	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.mcch_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.mcch_change_notification);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel9.scrambling_identity);

	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.cross_carrier_scheduling_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.carrier_indicator);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.srs_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.srs_request);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.antenna_ports_scrambling_and_layers);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.total_dci_length_including_padding);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel10.n_dl_rb);

	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel11.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel11.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel11.harq_ack_resource_offset);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel11.pdsch_re_mapping_quasi_co_location_indicator);

	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.primary_cell_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.number_ul_dl_configurations);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_indication[NFAPI_MAX_UL_DL_CONFIGURATIONS -1]);

	CU_ASSERT_EQUAL(in.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tl.tag, out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.laa_end_partial_sf_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.laa_end_partial_sf_configuration);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.initial_lbt_sf);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.codebook_size_determination);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.drms_table_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm_struct_flag);
	if(out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm_struct_flag)
	{
		
		IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_prb_per_subband);
		IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.number_of_subbands);
		IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_antennas);
		
		uint8_t sb_idx;
		for(sb_idx = 0; sb_idx < out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.number_of_subbands; ++sb_idx)
		{
			IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[sb_idx].subband_index);
			IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[sb_idx].scheduled_ues);
			
			uint8_t a_idx;
			uint8_t su_idx;
			
			for(a_idx = 0; a_idx < out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_antennas ; ++a_idx)
			{
				for(su_idx = 0; su_idx < out.dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[sb_idx].scheduled_ues; ++su_idx)
				{
					IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[0].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[sb_idx].precoding_value[a_idx][su_idx]);
				}
			}
		}
	}


	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[1].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[1].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[1].bch_pdu.bch_pdu_rel8.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[2].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[2].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[2].mch_pdu.mch_pdu_rel8.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel8.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel9.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel10.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel11.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel12.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[3].dlsch_pdu.dlsch_pdu_rel13.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[4].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[4].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[4].pch_pdu.pch_pdu_rel8.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[4].pch_pdu.pch_pdu_rel13.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[5].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[5].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[5].prs_pdu.prs_pdu_rel9.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[6].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[6].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[6].csi_rs_pdu.csi_rs_pdu_rel10.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[6].csi_rs_pdu.csi_rs_pdu_rel13.tl.tag);

	
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel8.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel9.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel10.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel11.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel12.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_params_rel11.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[7].epdcch_pdu.epdcch_params_rel13.tl.tag);

	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[8].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[8].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[8].mpdcch_pdu.mpdcch_pdu_rel13.tl.tag);
	
	//in.dl_config_request_body.dl_config_pdu_list[8].mpdcch_pdu;
	//
	
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.length);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.pdu_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.transmission_power);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[9].nbch_pdu.nbch_pdu_rel13.hyper_sfn_2_lsbs);
	
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.length);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.pdu_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.ncce_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.aggregation_level);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.start_symbol);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.rnti_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.rnti);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.scrambling_reinitialization_batch_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.dci_format);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.scheduling_delay);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.resource_assignment);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.repetition_number);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.mcs);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.new_data_indicator);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.harq_ack_resource);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.npdcch_order_indication);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.starting_number_of_nprach_repetitions);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.subcarrier_indication_of_nprach);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.paging_direct_indication_differentation_flag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.direct_indication);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.dci_subframe_repetition_number);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[10].npdcch_pdu.npdcch_pdu_rel13.total_dci_length_including_padding);
	
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].pdu_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].pdu_size);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.length);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.pdu_index);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.start_symbol);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.rnti_type);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.rnti);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.resource_assignment);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.repetition_number);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.modulation);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.number_of_subframes_for_resource_assignment);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.scrambling_sequence_initialization_cinit);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.sf_idx);
	IN_OUT_ASSERT(dl_config_request_body.dl_config_pdu_list[11].ndlsch_pdu.ndlsch_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue);
	
	
	free(in.dl_config_request_body.dl_config_pdu_list);
	free(out.dl_config_request_body.dl_config_pdu_list);
}
void nfapi_test_ul_config_request()
{
	nfapi_ul_config_request_t in;
	//memset(&in, 0, sizeof(in));
	nfapi_ul_config_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_UL_CONFIG_REQUEST;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 0x3FFF;

	in.ul_config_request_body.tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
	in.ul_config_request_body.number_of_pdus = 0;
	in.ul_config_request_body.rach_prach_frequency_resources = 34;
	in.ul_config_request_body.srs_present = 1;
	
	nfapi_ul_config_request_pdu_t ul_config_pdu_list[24];
	memset(&ul_config_pdu_list[0], 0, sizeof(ul_config_pdu_list));
	in.ul_config_request_body.ul_config_pdu_list = &ul_config_pdu_list[0];
	
	in.ul_config_request_body.ul_config_pdu_list[0].pdu_type = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[0].ulsch_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[1].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[1].ulsch_cqi_ri_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[2].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[2].ulsch_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[3].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[3].ulsch_cqi_harq_ri_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[4].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[4].uci_cqi_pdu;
	in.ul_config_request_body.number_of_pdus++;

	in.ul_config_request_body.ul_config_pdu_list[5].pdu_type = NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[5].uci_sr_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[6].pdu_type = NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[6].uci_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[7].pdu_type = NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[7].uci_sr_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[8].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[8].uci_cqi_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[9].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[9].uci_cqi_sr_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[10].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[10].uci_cqi_sr_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[11].pdu_type = NFAPI_UL_CONFIG_SRS_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[11].srs_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[12].pdu_type = NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[12].harq_buffer_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[13].pdu_type = NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[13].ulsch_uci_csi_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[14].pdu_type = NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[14].ulsch_uci_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[15].pdu_type = NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE;
	//in.ul_config_request_body.ul_config_pdu_list[15].ulsch_csi_uci_harq_pdu;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[16].pdu_type = NFAPI_UL_CONFIG_NULSCH_PDU_TYPE;
	in.ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_NULSCH_PDU_REL13_TAG;
	in.ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
	in.ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG;
	in.ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG;
	in.ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.nb_harq_information.nb_harq_information_rel13_fdd.tl.tag = NFAPI_UL_CONFIG_REQUEST_NB_HARQ_INFORMATION_REL13_FDD_TAG;
	in.ul_config_request_body.number_of_pdus++;
	
	in.ul_config_request_body.ul_config_pdu_list[17].pdu_type = NFAPI_UL_CONFIG_NRACH_PDU_TYPE;
	in.ul_config_request_body.ul_config_pdu_list[17].nrach_pdu.nrach_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_NRACH_PDU_REL13_TAG;
	in.ul_config_request_body.number_of_pdus++;
	


	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	IN_OUT_ASSERT(header.message_id);
	IN_OUT_ASSERT(sfn_sf);
	IN_OUT_ASSERT(ul_config_request_body.rach_prach_frequency_resources);
	IN_OUT_ASSERT(ul_config_request_body.srs_present);
	IN_OUT_ASSERT(ul_config_request_body.number_of_pdus);
	
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[0].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[1].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[2].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[3].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[4].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[5].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[6].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[7].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[8].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[9].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[10].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[11].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[12].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[13].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[14].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[15].pdu_type);
	
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].pdu_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.nulsch_format);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.handle);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.size);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.rnti);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.subcarrier_indication);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.resource_assignment);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.mcs);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.redudancy_version);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.repetition_number);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.new_data_indication);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.n_srs);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.scrambling_sequence_initialization_cinit);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.sf_idx);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.handle);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.rnti);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.virtual_cell_id_enabled_flag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.npusch_identity);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.ue_type);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.empty_symbols);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.total_number_of_repetitions);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.repetition_number);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.nb_harq_information.nb_harq_information_rel13_fdd.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[16].nulsch_pdu.nulsch_pdu_rel13.nb_harq_information.nb_harq_information_rel13_fdd.harq_ack_resource);
	
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[17].nrach_pdu.nrach_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[17].nrach_pdu.nrach_pdu_rel13.nprach_config_0);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[17].nrach_pdu.nrach_pdu_rel13.nprach_config_1);
	IN_OUT_ASSERT(ul_config_request_body.ul_config_pdu_list[17].nrach_pdu.nrach_pdu_rel13.nprach_config_2);
	

	free(out.ul_config_request_body.ul_config_pdu_list);
	
}
void nfapi_test_hi_dci0_request()
{
	nfapi_hi_dci0_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_hi_dci0_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_HI_DCI0_REQUEST;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 9999;
	in.hi_dci0_request_body.tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
	in.hi_dci0_request_body.sfnsf = 123;
	in.hi_dci0_request_body.number_of_dci = 0;
	in.hi_dci0_request_body.number_of_hi = 0;

	in.hi_dci0_request_body.hi_dci0_pdu_list = (nfapi_hi_dci0_request_pdu_t*)malloc(sizeof(nfapi_hi_dci0_request_pdu_t) * 8);

	in.hi_dci0_request_body.hi_dci0_pdu_list[0].pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.resource_block_start = 2;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = 23;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.hi_value = 0;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.i_phich = 2;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.transmission_power = 12312;;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL10_TAG;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel10.flag_tb2 = 1;
	in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel10.hi_value_2 = 2;

	in.hi_dci0_request_body.number_of_hi++;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].pdu_type = NFAPI_HI_DCI0_DCI_PDU_TYPE;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
	/*
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.dci_format;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.cce_index;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.aggregation_level;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.rnti;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.resource_block_start;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.number_of_resource_block;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.mcs_1;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.frequency_hopping_bits;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.new_data_indication_1;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.ue_tx_antenna_seleciton;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.tpc;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.cqi_csi_request;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.ul_index;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.dl_assignment_index;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.tpc_bitmap;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.transmission_power;
	*/
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL10_TAG;
	/*
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.cross_carrier_scheduling_flag;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.carrier_indicator;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.size_of_cqi_csi_feild;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.srs_flag;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.srs_request;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.resource_allocation_flag;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.resource_allocation_type;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.resource_block_coding;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.mcs_2;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.new_data_indication_2;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.number_of_antenna_ports;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.tpmi;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.total_dci_length_including_padding;
	*/
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel12.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL12_TAG;
	/*
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel12.pscch_resource;
	in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel12.time_resource_pattern;
	*/

	in.hi_dci0_request_body.number_of_dci++;

	in.hi_dci0_request_body.hi_dci0_pdu_list[2].pdu_type = NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE;
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL8_TAG;
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL10_TAG;
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].epdcch_dci_pdu.epdcch_parameters_rel11.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_PARAMETERS_REL11_TAG;
	/*
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].edpcch_dci_pdu.edpcch_dci_pdu_rel8;
	uint8_t dci_format;
	uint8_t cce_index;
	uint8_t aggregation_level;
	uint16_t rnti;
	uint8_t resource_block_start;
	uint8_t number_of_resource_block;
	uint8_t mcs_1;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t frequency_hopping_enabled_flag;
	uint8_t frequency_hopping_bits;
	uint8_t new_data_indication_1;
	uint8_t ue_tx_antenna_seleciton;
	uint8_t tpc;
	uint8_t cqi_csi_request;
	uint8_t ul_index;
	uint8_t dl_assignment_index;
	uint32_t tpc_bitmap;
	uint16_t transmission_power;
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].edpcch_dci_pdu.edpcch_dci_pdu_rel10.cross_carrier_scheduling_flag;
	uint8_t carrier_indicator;
	uint8_t size_of_cqi_csi_feild;
	uint8_t srs_flag;
	uint8_t srs_request;
	uint8_t resource_allocation_flag;
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t mcs_2;
	uint8_t new_data_indication_2;
	uint8_t number_of_antenna_ports;
	uint8_t tpmi;
	uint8_t total_dci_length_including_padding;
	in.hi_dci0_request_body.hi_dci0_pdu_list[2].edpcch_dci_pdu.edpcch_parameters_rel11.edpcch_resource_assigenment_flag;
	uint16_t edpcch_id;
	uint8_t epdcch_start_symbol;
	uint8_t epdcch_num_prb;
	uint8_t epdcch_prb_index[NFAPI_MAX_EPDCCH_PRB];
	nfapi_bf_vector_t bf_vector;
	*/
	in.hi_dci0_request_body.number_of_dci++;
	
	in.hi_dci0_request_body.hi_dci0_pdu_list[3].pdu_type = NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE;
	in.hi_dci0_request_body.hi_dci0_pdu_list[3].mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tl.tag = NFAPI_HI_DCI0_REQUEST_MPDCCH_DCI_PDU_REL13_TAG;
	/*
	in.hi_dci0_request_body.hi_dci0_pdu_list[3].mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.mpdcch_narrowband;
	uint8_t number_of_prb_pairs;
	uint8_t resource_block_assignment;
	uint8_t mpdcch_transmission_type;
	uint8_t start_symbol;
	uint8_t ecce_index;
	uint8_t aggreagation_level;
	uint8_t rnti_type;
	uint16_t rnti;
	uint8_t ce_mode;
	uint16_t drms_scrambling_init;
	uint16_t initial_transmission_sf_io;
	uint16_t transmission_power;
	uint8_t dci_format;
	uint8_t resource_block_start;
	uint8_t number_of_resource_blocks;
	uint8_t mcs;
	uint8_t pusch_repetition_levels;
	uint8_t frequency_hopping_flag;
	uint8_t new_data_indication;
	uint8_t harq_process;
	uint8_t redudency_version;
	uint8_t tpc;
	uint8_t csi_request;
	uint8_t ul_inex;
	uint8_t dai_presence_flag;
	uint8_t dl_assignment_index;
	uint8_t srs_request;
	uint8_t dci_subframe_repetition_number;
	uint32_t tcp_bitmap;
	uint8_t total_dci_length_include_padding;
	uint8_t number_of_tx_antenna_ports;
	uint16_t precoding_value[NFAPI_MAX_ANTENNA_PORT_COUNT];
	*/
	in.hi_dci0_request_body.number_of_dci++;

	in.hi_dci0_request_body.hi_dci0_pdu_list[4].pdu_type = NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE;
	in.hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.tl.tag = NFAPI_HI_DCI0_REQUEST_NPDCCH_DCI_PDU_REL13_TAG;
	in.hi_dci0_request_body.number_of_dci++;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.tl.tag, out.hi_dci0_request_body.tl.tag);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.sfnsf, out.hi_dci0_request_body.sfnsf);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.number_of_dci, out.hi_dci0_request_body.number_of_dci);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.number_of_hi, out.hi_dci0_request_body.number_of_hi);

	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[0].pdu_type, out.hi_dci0_request_body.hi_dci0_pdu_list[0].pdu_type);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.tl.tag, out.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel10.tl.tag, out.hi_dci0_request_body.hi_dci0_pdu_list[0].hi_pdu.hi_pdu_rel10.tl.tag);

	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[1].pdu_type, out.hi_dci0_request_body.hi_dci0_pdu_list[1].pdu_type);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.tl.tag, out.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.tl.tag, out.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel10.tl.tag);
	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel12.tl.tag, out.hi_dci0_request_body.hi_dci0_pdu_list[1].dci_pdu.dci_pdu_rel12.tl.tag);

	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[2].pdu_type, out.hi_dci0_request_body.hi_dci0_pdu_list[2].pdu_type);

	CU_ASSERT_EQUAL(in.hi_dci0_request_body.hi_dci0_pdu_list[3].pdu_type, out.hi_dci0_request_body.hi_dci0_pdu_list[3].pdu_type);
	
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].pdu_type);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.tl.tag);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.ncce_index);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.aggregation_level);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.start_symbol);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.rnti);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.scrambling_reinitialization_batch_index);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.subcarrier_indication);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.resource_assignment);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.scheduling_delay);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.mcs);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.redudancy_version);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.repetition_number);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.new_data_indicator);
	IN_OUT_ASSERT(hi_dci0_request_body.hi_dci0_pdu_list[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.dci_subframe_repetition_number);

	free(in.hi_dci0_request_body.hi_dci0_pdu_list);
	free(out.hi_dci0_request_body.hi_dci0_pdu_list);
}
void nfapi_test_tx_request()
{
	nfapi_tx_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_tx_request_t out;

	nfapi_p7_codec_config_t config;
	config.allocate = &nfapi_allocate_pdu;
	//config.deallocate = &nfapi_deallocate_pdu;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_TX_REQUEST;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	uint8_t pdu1_seg1[16];
	uint8_t pdu1_seg2[16];
	uint8_t pdu2[8];

	in.sfn_sf = 10230;
	in.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
	in.tx_request_body.number_of_pdus = 2;

	in.tx_request_body.tx_pdu_list = (nfapi_tx_request_pdu_t*)(malloc(sizeof(nfapi_tx_request_pdu_t) * in.tx_request_body.number_of_pdus));

	in.tx_request_body.tx_pdu_list[0].pdu_length = sizeof(pdu1_seg1) + sizeof(pdu1_seg2);
	in.tx_request_body.tx_pdu_list[0].pdu_index = 0;
	in.tx_request_body.tx_pdu_list[0].num_segments = 2;
	in.tx_request_body.tx_pdu_list[0].segments[0].segment_length = sizeof(pdu1_seg1);
	in.tx_request_body.tx_pdu_list[0].segments[0].segment_data = pdu1_seg1;
	in.tx_request_body.tx_pdu_list[0].segments[1].segment_length = sizeof(pdu1_seg2);
	in.tx_request_body.tx_pdu_list[0].segments[1].segment_data = pdu1_seg2;

	in.tx_request_body.tx_pdu_list[1].pdu_length = sizeof(pdu2);
	in.tx_request_body.tx_pdu_list[1].pdu_index = 0;
	in.tx_request_body.tx_pdu_list[1].num_segments = 1;
	in.tx_request_body.tx_pdu_list[1].segments[0].segment_length = sizeof(pdu2);
	in.tx_request_body.tx_pdu_list[1].segments[0].segment_data = pdu2;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, &config);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), &config);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	CU_ASSERT_EQUAL(in.tx_request_body.tl.tag, out.tx_request_body.tl.tag);
	CU_ASSERT_EQUAL(in.tx_request_body.number_of_pdus, out.tx_request_body.number_of_pdus);
	CU_ASSERT_EQUAL(in.tx_request_body.tx_pdu_list[0].pdu_length, out.tx_request_body.tx_pdu_list[0].pdu_length);
	CU_ASSERT_EQUAL(1, out.tx_request_body.tx_pdu_list[0].num_segments);

	CU_ASSERT_EQUAL(in.tx_request_body.tx_pdu_list[1].pdu_length, out.tx_request_body.tx_pdu_list[1].pdu_length);
	CU_ASSERT_EQUAL(1, out.tx_request_body.tx_pdu_list[1].num_segments);

	CU_ASSERT_EQUAL(0, memcmp(pdu2, out.tx_request_body.tx_pdu_list[1].segments[0].segment_data, sizeof(pdu2)));

	free(out.tx_request_body.tx_pdu_list[0].segments[0].segment_data);
	free(out.tx_request_body.tx_pdu_list[1].segments[0].segment_data);

	free(in.tx_request_body.tx_pdu_list);
	free(out.tx_request_body.tx_pdu_list);
}
void nfapi_test_harq_indication()
{
	nfapi_harq_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_harq_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_HARQ_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10230;
	in.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
	in.harq_indication_body.number_of_harqs = 6;

	in.harq_indication_body.harq_pdu_list = (nfapi_harq_indication_pdu_t*)(calloc(1, sizeof(nfapi_harq_indication_pdu_t) * in.harq_indication_body.number_of_harqs));

	in.harq_indication_body.harq_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL8_TAG;
	in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.mode = 0;
	in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.number_of_ack_nack = 2;
	in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.harq_data.bundling.value_0 = 6;
	in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.harq_data.bundling.value_1 = 6;

	in.harq_indication_body.harq_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL9_TAG;
	in.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.mode = 0;
	in.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.number_of_ack_nack = 2;
	in.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.harq_data[0].bundling.value_0 = 2;

	in.harq_indication_body.harq_pdu_list[2].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
	in.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.mode = 2;
	in.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.number_of_ack_nack = 2;
	in.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.harq_data[0].special_bundling.value_0 = 2;

	in.harq_indication_body.harq_pdu_list[3].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[3].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[3].ul_cqi_information.ul_cqi = 9;
	in.harq_indication_body.harq_pdu_list[3].ul_cqi_information.channel = 3;
	in.harq_indication_body.harq_pdu_list[3].harq_indication_fdd_rel8.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL8_TAG;
	in.harq_indication_body.harq_pdu_list[3].harq_indication_fdd_rel8.harq_tb1 = 1;
	in.harq_indication_body.harq_pdu_list[3].harq_indication_fdd_rel8.harq_tb2 = 2;


	in.harq_indication_body.harq_pdu_list[4].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL9_TAG;
	in.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.mode = 0;
	in.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.number_of_ack_nack = 3;
	in.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.harq_tb_n[0] = 45;

	in.harq_indication_body.harq_pdu_list[5].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
	in.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.mode = 1;
	in.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.number_of_ack_nack = 22;
	in.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.harq_tb_n[0] = 123;


	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);

	CU_ASSERT_EQUAL(in.harq_indication_body.tl.tag,out.harq_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.number_of_harqs,out.harq_indication_body.number_of_harqs);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[0].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.tl.tag,out.harq_indication_body.harq_pdu_list[0].harq_indication_tdd_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[1].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[1].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.tl.tag,out.harq_indication_body.harq_pdu_list[1].harq_indication_tdd_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[2].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[2].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.tl.tag,out.harq_indication_body.harq_pdu_list[2].harq_indication_tdd_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[3].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[3].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[3].ul_cqi_information.tl.tag,out.harq_indication_body.harq_pdu_list[3].ul_cqi_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[3].harq_indication_fdd_rel8.tl.tag,out.harq_indication_body.harq_pdu_list[3].harq_indication_fdd_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[4].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[4].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.tl.tag,out.harq_indication_body.harq_pdu_list[4].harq_indication_fdd_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[5].rx_ue_information.tl.tag,out.harq_indication_body.harq_pdu_list[5].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.tl.tag,out.harq_indication_body.harq_pdu_list[5].harq_indication_fdd_rel13.tl.tag);

	free(in.harq_indication_body.harq_pdu_list);
	free(out.harq_indication_body.harq_pdu_list);
}
void nfapi_test_crc_indication()
{
	nfapi_crc_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_crc_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_CRC_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10230;
	in.crc_indication_body.tl.tag = NFAPI_CRC_INDICATION_BODY_TAG;
	in.crc_indication_body.number_of_crcs = 2;
	in.crc_indication_body.crc_pdu_list = (nfapi_crc_indication_pdu_t*)malloc(sizeof(nfapi_crc_indication_pdu_t) * in.crc_indication_body.number_of_crcs);
	in.crc_indication_body.crc_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.crc_indication_body.crc_pdu_list[0].rx_ue_information.handle = 0x4567;
	in.crc_indication_body.crc_pdu_list[0].rx_ue_information.rnti = 42;
	in.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.tl.tag = NFAPI_CRC_INDICATION_REL8_TAG;
	in.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.crc_flag = 0;


	in.crc_indication_body.crc_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.crc_indication_body.crc_pdu_list[1].rx_ue_information.handle = 0x4568;
	in.crc_indication_body.crc_pdu_list[1].rx_ue_information.rnti = 43;
	in.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.tl.tag = NFAPI_CRC_INDICATION_REL8_TAG;
	in.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.crc_flag = 1;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.crc_indication_body.tl.tag, out.crc_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.crc_indication_body.number_of_crcs, out.crc_indication_body.number_of_crcs);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[0].rx_ue_information.tl.tag, out.crc_indication_body.crc_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[0].rx_ue_information.handle, out.crc_indication_body.crc_pdu_list[0].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[0].rx_ue_information.rnti, out.crc_indication_body.crc_pdu_list[0].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.tl.tag, out.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.crc_flag, out.crc_indication_body.crc_pdu_list[0].crc_indication_rel8.crc_flag);

	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[1].rx_ue_information.tl.tag, out.crc_indication_body.crc_pdu_list[1].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[1].rx_ue_information.handle, out.crc_indication_body.crc_pdu_list[1].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[1].rx_ue_information.rnti, out.crc_indication_body.crc_pdu_list[1].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.tl.tag, out.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.crc_flag, out.crc_indication_body.crc_pdu_list[1].crc_indication_rel8.crc_flag);

	free(in.crc_indication_body.crc_pdu_list);
	free(out.crc_indication_body.crc_pdu_list);
}
void nfapi_test_rx_ulsch_indication()
{
	nfapi_rx_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_rx_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RX_ULSCH_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	uint8_t pdu1[32];
	uint8_t pdu2[8];

	in.sfn_sf = 12354;
	in.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
	in.rx_indication_body.number_of_pdus = 3;

	in.rx_indication_body.rx_pdu_list = (nfapi_rx_indication_pdu_t*)(calloc(1, sizeof(nfapi_rx_indication_pdu_t) * in.rx_indication_body.number_of_pdus));
	
	in.rx_indication_body.rx_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.rx_indication_body.rx_pdu_list[0].rx_ue_information.handle = 0xFEDC;
	in.rx_indication_body.rx_pdu_list[0].rx_ue_information.rnti = 42;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.length = sizeof(pdu1);
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.offset = 1;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.ul_cqi = 1;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.timing_advance = 23;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.tl.tag = NFAPI_RX_INDICATION_REL9_TAG;
	in.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.timing_advance_r9 = 1;
	in.rx_indication_body.rx_pdu_list[0].data = pdu1;

	in.rx_indication_body.rx_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.rx_indication_body.rx_pdu_list[1].rx_ue_information.handle = 0xFEDC;
	in.rx_indication_body.rx_pdu_list[1].rx_ue_information.rnti = 43;
	in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
	in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.length = 0;
	in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.offset = 1;
	in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.ul_cqi = 1;
	in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.timing_advance = 23;
	in.rx_indication_body.rx_pdu_list[1].data = 0;

	in.rx_indication_body.rx_pdu_list[2].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.rx_indication_body.rx_pdu_list[2].rx_ue_information.handle = 0xFEDC;
	in.rx_indication_body.rx_pdu_list[2].rx_ue_information.rnti = 43;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.tl.tag = NFAPI_RX_INDICATION_REL8_TAG;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.length = sizeof(pdu2);
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.offset = 1;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.ul_cqi = 1;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.timing_advance = 23;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.tl.tag = NFAPI_RX_INDICATION_REL9_TAG;
	in.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.timing_advance_r9 = 34;
	in.rx_indication_body.rx_pdu_list[2].data = pdu2;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	CU_ASSERT_EQUAL(in.rx_indication_body.tl.tag, out.rx_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.number_of_pdus, out.rx_indication_body.number_of_pdus);
	
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_ue_information.tl.tag, out.rx_indication_body.rx_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_ue_information.handle, out.rx_indication_body.rx_pdu_list[0].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_ue_information.rnti, out.rx_indication_body.rx_pdu_list[0].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.tl.tag, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.length, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.length);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.offset, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.offset);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.ul_cqi, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.ul_cqi);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.timing_advance, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel8.timing_advance);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.tl.tag, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.timing_advance_r9, out.rx_indication_body.rx_pdu_list[0].rx_indication_rel9.timing_advance_r9);
	CU_ASSERT_EQUAL(0, memcmp(in.rx_indication_body.rx_pdu_list[0].data, out.rx_indication_body.rx_pdu_list[0].data, sizeof(pdu1)));

	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_ue_information.tl.tag, out.rx_indication_body.rx_pdu_list[1].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_ue_information.handle, out.rx_indication_body.rx_pdu_list[1].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_ue_information.rnti, out.rx_indication_body.rx_pdu_list[1].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.tl.tag, out.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.length, out.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.length);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.offset, out.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.offset);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.ul_cqi, out.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.ul_cqi);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.timing_advance, out.rx_indication_body.rx_pdu_list[1].rx_indication_rel8.timing_advance);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[1].data, 0);

	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_ue_information.tl.tag, out.rx_indication_body.rx_pdu_list[2].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_ue_information.handle, out.rx_indication_body.rx_pdu_list[2].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_ue_information.rnti, out.rx_indication_body.rx_pdu_list[2].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.tl.tag, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.length, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.length);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.offset, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.offset);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.ul_cqi, in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.ul_cqi);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.timing_advance, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel8.timing_advance);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.tl.tag, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.timing_advance_r9, out.rx_indication_body.rx_pdu_list[2].rx_indication_rel9.timing_advance_r9);
	CU_ASSERT_EQUAL(0, memcmp(in.rx_indication_body.rx_pdu_list[2].data, out.rx_indication_body.rx_pdu_list[2].data, sizeof(pdu2)));

	free(in.rx_indication_body.rx_pdu_list);
	free(out.rx_indication_body.rx_pdu_list);

}
void nfapi_test_rach_indication()
{
	nfapi_rach_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_rach_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RACH_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10230;
	in.rach_indication_body.tl.tag = NFAPI_RACH_INDICATION_BODY_TAG;
	in.rach_indication_body.number_of_preambles = 2;
	in.rach_indication_body.preamble_list = (nfapi_preamble_pdu_t*)(malloc(sizeof(nfapi_preamble_pdu_t)*in.rach_indication_body.number_of_preambles));
	in.rach_indication_body.preamble_list[0].preamble_rel8.tl.tag = NFAPI_PREAMBLE_REL8_TAG;
	in.rach_indication_body.preamble_list[0].preamble_rel9.tl.tag = NFAPI_PREAMBLE_REL9_TAG;
	in.rach_indication_body.preamble_list[0].preamble_rel13.tl.tag = NFAPI_PREAMBLE_REL13_TAG;

	in.rach_indication_body.preamble_list[1].preamble_rel8.tl.tag = NFAPI_PREAMBLE_REL8_TAG;
	in.rach_indication_body.preamble_list[1].preamble_rel9.tl.tag = NFAPI_PREAMBLE_REL9_TAG;
	in.rach_indication_body.preamble_list[1].preamble_rel13.tl.tag = NFAPI_PREAMBLE_REL13_TAG;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.rach_indication_body.tl.tag, out.rach_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.number_of_preambles, out.rach_indication_body.number_of_preambles);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[0].preamble_rel8.tl.tag, out.rach_indication_body.preamble_list[0].preamble_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[0].preamble_rel9.tl.tag, out.rach_indication_body.preamble_list[0].preamble_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[0].preamble_rel13.tl.tag, out.rach_indication_body.preamble_list[0].preamble_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[1].preamble_rel8.tl.tag, out.rach_indication_body.preamble_list[1].preamble_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[1].preamble_rel9.tl.tag, out.rach_indication_body.preamble_list[1].preamble_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.rach_indication_body.preamble_list[1].preamble_rel13.tl.tag, out.rach_indication_body.preamble_list[1].preamble_rel13.tl.tag);

	free(in.rach_indication_body.preamble_list);
	free(out.rach_indication_body.preamble_list);
}
void nfapi_test_srs_indication()
{
	nfapi_srs_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_srs_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_SRS_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10230;
	in.srs_indication_body.tl.tag = NFAPI_SRS_INDICATION_BODY_TAG;
	in.srs_indication_body.number_of_ues = 2;
	in.srs_indication_body.srs_pdu_list = (nfapi_srs_indication_pdu_t*)(malloc(sizeof(nfapi_srs_indication_pdu_t) * in.srs_indication_body.number_of_ues));
	in.srs_indication_body.srs_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.srs_indication_body.srs_pdu_list[0].rx_ue_information.handle = 0x4567;
	in.srs_indication_body.srs_pdu_list[0].rx_ue_information.rnti = 42;

	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.tl.tag = NFAPI_SRS_INDICATION_FDD_REL8_TAG;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.doppler_estimation = 244;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.timing_advance = 45;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.number_of_resource_blocks = 100;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.rb_start = 0;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.snr[0] = 255;
	//...
	
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.tl.tag = NFAPI_SRS_INDICATION_FDD_REL9_TAG;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.timing_advance_r9 = 7690;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.tl.tag = NFAPI_SRS_INDICATION_TDD_REL10_TAG;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.uppts_symbol = 1;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.tl.tag = NFAPI_SRS_INDICATION_FDD_REL11_TAG;
	in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.ul_rtoa = 4800;

	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.tl.tag = NFAPI_TDD_CHANNEL_MEASUREMENT_TAG;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_prb_per_subband = 4;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.number_of_subbands = 2;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_atennas = 2;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].subband_index = 4;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].channel[0] = 0xEEEE;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].subband_index = 6;
	in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].channel[0] = 0xAAAA;

	in.srs_indication_body.srs_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.srs_indication_body.srs_pdu_list[1].rx_ue_information.handle = 0x4567;
	in.srs_indication_body.srs_pdu_list[1].rx_ue_information.rnti = 42;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

	CU_ASSERT_EQUAL(in.srs_indication_body.tl.tag, out.srs_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].rx_ue_information.tl.tag, out.srs_indication_body.srs_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].rx_ue_information.handle, out.srs_indication_body.srs_pdu_list[0].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].rx_ue_information.rnti, out.srs_indication_body.srs_pdu_list[0].rx_ue_information.rnti);

	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.tl.tag, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.doppler_estimation, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.doppler_estimation);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.timing_advance, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.timing_advance);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.number_of_resource_blocks, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.number_of_resource_blocks);
	CU_ASSERT_EQUAL(memcmp(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.snr, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.snr, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.number_of_resource_blocks), 0);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.rb_start, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel8.rb_start);

	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.tl.tag, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.timing_advance_r9, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel9.timing_advance_r9);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.tl.tag, out.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.uppts_symbol, out.srs_indication_body.srs_pdu_list[0].srs_indication_tdd_rel10.uppts_symbol);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.tl.tag, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.ul_rtoa, out.srs_indication_body.srs_pdu_list[0].srs_indication_fdd_rel11.ul_rtoa);

	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.tl.tag, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_prb_per_subband, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_prb_per_subband);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.number_of_subbands, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.number_of_subbands);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_atennas, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.num_atennas);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].subband_index, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].subband_index) 
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].channel[0], out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[0].channel[0]);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].subband_index, out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].subband_index);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].channel[0], out.srs_indication_body.srs_pdu_list[0].tdd_channel_measurement.subands[1].channel[0]);


	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[1].rx_ue_information.tl.tag, out.srs_indication_body.srs_pdu_list[1].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[1].rx_ue_information.handle, out.srs_indication_body.srs_pdu_list[1].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.srs_indication_body.srs_pdu_list[1].rx_ue_information.rnti, out.srs_indication_body.srs_pdu_list[1].rx_ue_information.rnti);

	free(in.srs_indication_body.srs_pdu_list);
	free(out.srs_indication_body.srs_pdu_list);
}
void nfapi_test_rx_sr_indication()
{
	nfapi_sr_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_sr_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RX_SR_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10230;
	in.sr_indication_body.tl.tag = NFAPI_SR_INDICATION_BODY_TAG;
	in.sr_indication_body.number_of_srs = 2;
	in.sr_indication_body.sr_pdu_list = (nfapi_sr_indication_pdu_t*)(malloc(sizeof(nfapi_sr_indication_pdu_t) * in.sr_indication_body.number_of_srs));
	in.sr_indication_body.sr_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.sr_indication_body.sr_pdu_list[0].rx_ue_information.handle = 0x4567;
	in.sr_indication_body.sr_pdu_list[0].rx_ue_information.rnti = 42;

	in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.ul_cqi = 34;
	in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.channel = 38;

	in.sr_indication_body.sr_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.sr_indication_body.sr_pdu_list[1].rx_ue_information.handle = 0x9876;
	in.sr_indication_body.sr_pdu_list[1].rx_ue_information.rnti = 24;

	in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.ul_cqi = 24;
	in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.channel = 28;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

	CU_ASSERT_EQUAL(in.sr_indication_body.tl.tag, out.sr_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].rx_ue_information.tl.tag, out.sr_indication_body.sr_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].rx_ue_information.handle, out.sr_indication_body.sr_pdu_list[0].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].rx_ue_information.rnti, out.sr_indication_body.sr_pdu_list[0].rx_ue_information.rnti);

	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.tl.tag, out.sr_indication_body.sr_pdu_list[0].ul_cqi_information.tl.tag);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.ul_cqi, out.sr_indication_body.sr_pdu_list[0].ul_cqi_information.ul_cqi);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[0].ul_cqi_information.channel, out.sr_indication_body.sr_pdu_list[0].ul_cqi_information.channel);

	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].rx_ue_information.tl.tag, out.sr_indication_body.sr_pdu_list[1].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].rx_ue_information.handle, out.sr_indication_body.sr_pdu_list[1].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].rx_ue_information.rnti, out.sr_indication_body.sr_pdu_list[1].rx_ue_information.rnti);

	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.tl.tag, out.sr_indication_body.sr_pdu_list[1].ul_cqi_information.tl.tag);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.ul_cqi, out.sr_indication_body.sr_pdu_list[1].ul_cqi_information.ul_cqi);
	CU_ASSERT_EQUAL(in.sr_indication_body.sr_pdu_list[1].ul_cqi_information.channel, out.sr_indication_body.sr_pdu_list[1].ul_cqi_information.channel);

	free(in.sr_indication_body.sr_pdu_list);
	free(out.sr_indication_body.sr_pdu_list);

}
void nfapi_test_rx_cqi_indication()
{
	uint8_t cqi_1[] = { 0, 2, 4, 8, 10};
	uint8_t cqi_2[] = { 100, 101, 102, 103, 104, 105, 106};

	nfapi_cqi_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_cqi_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_RX_CQI_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 10240;
	in.cqi_indication_body.tl.tag = NFAPI_CQI_INDICATION_BODY_TAG;
	in.cqi_indication_body.number_of_cqis = 3;

	in.cqi_indication_body.cqi_pdu_list = (nfapi_cqi_indication_pdu_t*)(calloc(1, sizeof(nfapi_cqi_indication_pdu_t)*in.cqi_indication_body.number_of_cqis));
	in.cqi_indication_body.cqi_raw_pdu_list = (nfapi_cqi_indication_raw_pdu_t*)(calloc(1, sizeof(nfapi_cqi_indication_raw_pdu_t)*in.cqi_indication_body.number_of_cqis));

	in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.handle = 0x4567;
	in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.rnti = 42;
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.tl.tag = NFAPI_CQI_INDICATION_REL8_TAG;
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.length = sizeof(cqi_1);
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.data_offset = 1;
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ul_cqi = 4;
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ri = 4;
	in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.timing_advance = 63;
	in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.ul_cqi = 34;
	in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.channel = 38;

	in.cqi_indication_body.cqi_pdu_list[1].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.cqi_indication_body.cqi_pdu_list[1].rx_ue_information.handle = 0x4567;
	in.cqi_indication_body.cqi_pdu_list[1].rx_ue_information.rnti = 42;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.tl.tag = NFAPI_CQI_INDICATION_REL8_TAG;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.length = 0;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.data_offset = 0;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.ul_cqi = 0;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.ri = 0;
	in.cqi_indication_body.cqi_pdu_list[1].cqi_indication_rel8.timing_advance = 0;


	in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.handle = 0x4567;
	in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.rnti = 42;
	
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.tl.tag = NFAPI_CQI_INDICATION_REL9_TAG;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.length = sizeof(cqi_2);
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.data_offset = 1;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ul_cqi = 4;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.number_of_cc_reported = 4;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[0] = 6;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[1] = 7;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[2] = 8;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[3] = 1;
	in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.timing_advance = 63;
	in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.ul_cqi = 44;
	in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.channel = 48;


	memcpy(in.cqi_indication_body.cqi_raw_pdu_list[0].pdu, cqi_1, sizeof(cqi_1));
	memcpy(in.cqi_indication_body.cqi_raw_pdu_list[1].pdu, cqi_2, sizeof(cqi_2));

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);

	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	CU_ASSERT_EQUAL(in.cqi_indication_body.tl.tag, out.cqi_indication_body.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.tl.tag, out.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.handle, out.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.rnti, out.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.rnti);

	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.tl.tag, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.length, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.length);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.data_offset, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.data_offset);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ul_cqi, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ul_cqi);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ri, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.ri);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.timing_advance, out.cqi_indication_body.cqi_pdu_list[0].cqi_indication_rel8.timing_advance);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.tl.tag, out.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.ul_cqi, out.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.ul_cqi);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.channel, out.cqi_indication_body.cqi_pdu_list[0].ul_cqi_information.channel);

	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.tl.tag, out.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.handle, out.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.handle);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.rnti, out.cqi_indication_body.cqi_pdu_list[2].rx_ue_information.rnti);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.tl.tag, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.length, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.length);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.data_offset, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.data_offset);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ul_cqi, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ul_cqi);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.number_of_cc_reported, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.number_of_cc_reported);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[0], out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[0]);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[1], out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[1]);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[2], out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[2]);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[3], out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.ri[3]);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.timing_advance, out.cqi_indication_body.cqi_pdu_list[2].cqi_indication_rel9.timing_advance);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.tl.tag, out.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.tl.tag);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.ul_cqi, out.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.ul_cqi);
	CU_ASSERT_EQUAL(in.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.channel, out.cqi_indication_body.cqi_pdu_list[2].ul_cqi_information.channel);


	CU_ASSERT_EQUAL(memcmp(in.cqi_indication_body.cqi_raw_pdu_list[0].pdu, out.cqi_indication_body.cqi_raw_pdu_list[0].pdu, sizeof(cqi_1)), 0);
	CU_ASSERT_EQUAL(memcmp(in.cqi_indication_body.cqi_raw_pdu_list[2].pdu, out.cqi_indication_body.cqi_raw_pdu_list[2].pdu, sizeof(cqi_2)), 0);

	free(in.cqi_indication_body.cqi_pdu_list);
	free(in.cqi_indication_body.cqi_raw_pdu_list);
	free(out.cqi_indication_body.cqi_pdu_list);
	free(out.cqi_indication_body.cqi_raw_pdu_list);
}

void nfapi_test_lbt_dl_config_request()
{
	nfapi_lbt_dl_config_request_t in;
	memset(&in, 0, sizeof(in));
	nfapi_lbt_dl_config_request_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_LBT_DL_CONFIG_REQUEST;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	//in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 1234;
	in.lbt_dl_config_request_body.tl.tag = NFAPI_LBT_DL_CONFIG_REQUEST_BODY_TAG;
	in.lbt_dl_config_request_body.number_of_pdus = 2;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list = (nfapi_lbt_dl_config_request_pdu_t*)(malloc(sizeof(nfapi_lbt_dl_config_request_pdu_t) * in.lbt_dl_config_request_body.number_of_pdus));
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_type = NFAPI_LBT_DL_CONFIG_REQUEST_PDSCH_PDU_TYPE;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_size = 32;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.tl.tag = NFAPI_LBT_PDSCH_REQ_PDU_REL13_TAG;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.handle = 0x1234;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.mp_cca = 22;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.n_cca = 23;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.offset = 24;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lte_txop_sf = 25;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.txop_sfn_sf_end = 26;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lbt_mode = 27;

	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_type = NFAPI_LBT_DL_CONFIG_REQUEST_DRS_PDU_TYPE;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_size = 32;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.tl.tag = NFAPI_LBT_DRS_REQ_PDU_REL13_TAG;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.handle = 0x4567;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.offset = 1;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.sfn_sf_end = 2;
	in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.lbt_mode = 3;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.phy_id, out.header.phy_id);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.header.message_length, out.header.message_length);
	CU_ASSERT_EQUAL(in.header.m_segment_sequence, out.header.m_segment_sequence);
	CU_ASSERT_EQUAL(in.header.checksum, out.header.checksum);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);

	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.number_of_pdus, out.lbt_dl_config_request_body.number_of_pdus);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_type, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_type);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_size, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].pdu_size);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.tl.tag, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.handle, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.handle);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.mp_cca, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.mp_cca);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.n_cca, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.n_cca);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.offset, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.offset);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lte_txop_sf, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lte_txop_sf);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.txop_sfn_sf_end, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.txop_sfn_sf_end);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lbt_mode, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[0].lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13.lbt_mode);

	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_type, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_type);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_size, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].pdu_size);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.tl.tag, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.handle, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.handle);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.offset, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.offset);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.sfn_sf_end, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.sfn_sf_end);
	CU_ASSERT_EQUAL(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.lbt_mode, out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list[1].lbt_drs_req_pdu.lbt_drs_req_pdu_rel13.lbt_mode);

	free(in.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list);
	free(out.lbt_dl_config_request_body.lbt_dl_config_req_pdu_list);
}

void nfapi_test_lbt_dl_indication()
{
	nfapi_lbt_dl_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_lbt_dl_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_LBT_DL_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	//in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 1234;
	in.lbt_dl_indication_body.tl.tag = NFAPI_LBT_DL_INDICATION_BODY_TAG;
	in.lbt_dl_indication_body.number_of_pdus = 2;
	in.lbt_dl_indication_body.lbt_indication_pdu_list = (nfapi_lbt_dl_indication_pdu_t*)(malloc(sizeof(nfapi_lbt_dl_indication_pdu_t) * in.lbt_dl_indication_body.number_of_pdus));
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_type = NFAPI_LBT_DL_RSP_PDSCH_PDU_TYPE;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_size = 32;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.tl.tag = NFAPI_LBT_PDSCH_RSP_PDU_REL13_TAG;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.handle = 0x1234;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.result = 22;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.lte_txop_symbols = 23;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.initial_partial_sf = 24;

	in.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_type = NFAPI_LBT_DL_RSP_DRS_PDU_TYPE;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_size = 32;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.tl.tag = NFAPI_LBT_DRS_RSP_PDU_REL13_TAG;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.handle = 0x4567;
	in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.result = 1;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.phy_id, out.header.phy_id);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.header.message_length, out.header.message_length);
	CU_ASSERT_EQUAL(in.header.m_segment_sequence, out.header.m_segment_sequence);
	CU_ASSERT_EQUAL(in.header.checksum, out.header.checksum);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);

	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.number_of_pdus, out.lbt_dl_indication_body.number_of_pdus);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_type, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_type);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_size, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_size);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.tl.tag, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.handle, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.handle);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.result, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.result);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.lte_txop_symbols, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.lte_txop_symbols);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.initial_partial_sf, out.lbt_dl_indication_body.lbt_indication_pdu_list[0].lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13.initial_partial_sf);

	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_type, out.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_type);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_size, out.lbt_dl_indication_body.lbt_indication_pdu_list[1].pdu_size);

	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.tl.tag, out.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.tl.tag);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.handle, out.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.handle);
	CU_ASSERT_EQUAL(in.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.result, out.lbt_dl_indication_body.lbt_indication_pdu_list[1].lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13.result);

	free(in.lbt_dl_indication_body.lbt_indication_pdu_list);
	free(out.lbt_dl_indication_body.lbt_indication_pdu_list);

}
void nfapi_test_lbt_dl_indication_invalid_pdu_type()
{
	nfapi_lbt_dl_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_lbt_dl_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_LBT_DL_INDICATION;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	//in.header.checksum = 0xEEEEEEEE;

	in.sfn_sf = 1234;
	in.lbt_dl_indication_body.tl.tag = NFAPI_LBT_DL_INDICATION_BODY_TAG;
	in.lbt_dl_indication_body.number_of_pdus = 1;
	in.lbt_dl_indication_body.lbt_indication_pdu_list = (nfapi_lbt_dl_indication_pdu_t*)(malloc(sizeof(nfapi_lbt_dl_indication_pdu_t) * in.lbt_dl_indication_body.number_of_pdus));
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_type = 44; // Invalid pdu type
	in.lbt_dl_indication_body.lbt_indication_pdu_list[0].pdu_size = 32;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	int unpack_result = nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(unpack_result, -1);
	CU_ASSERT_EQUAL(in.header.phy_id, out.header.phy_id);
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.header.message_length, out.header.message_length);
	CU_ASSERT_EQUAL(in.header.m_segment_sequence, out.header.m_segment_sequence);
	CU_ASSERT_EQUAL(in.header.checksum, out.header.checksum);
	CU_ASSERT_EQUAL(in.sfn_sf, out.sfn_sf);
	free(in.lbt_dl_indication_body.lbt_indication_pdu_list);
	free(out.lbt_dl_indication_body.lbt_indication_pdu_list);
}

void nfapi_test_nb_harq_indication()
{
	nfapi_nb_harq_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_nb_harq_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_NB_HARQ_INDICATION;
	in.header.message_length = 0;

	in.sfn_sf = 1234;
	in.nb_harq_indication_body.tl.tag = NFAPI_NB_HARQ_INDICATION_BODY_TAG;
	in.nb_harq_indication_body.number_of_harqs = 1;
	
	nfapi_nb_harq_indication_pdu_t nb_harq_pdu[1];
	in.nb_harq_indication_body.nb_harq_pdu_list = &nb_harq_pdu[0];
	in.nb_harq_indication_body.nb_harq_pdu_list[0].rx_ue_information.tl.tag = NFAPI_RX_UE_INFORMATION_TAG;
	in.nb_harq_indication_body.nb_harq_pdu_list[0].nb_harq_indication_fdd_rel13.tl.tag = NFAPI_NB_HARQ_INDICATION_FDD_REL13_TAG;
	in.nb_harq_indication_body.nb_harq_pdu_list[0].ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;
	

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	IN_OUT_ASSERT(sfn_sf);
	IN_OUT_ASSERT(nb_harq_indication_body.tl.tag);
	IN_OUT_ASSERT(nb_harq_indication_body.number_of_harqs);
	
	IN_OUT_ASSERT(nb_harq_indication_body.nb_harq_pdu_list[0].rx_ue_information.tl.tag);
	IN_OUT_ASSERT(nb_harq_indication_body.nb_harq_pdu_list[0].nb_harq_indication_fdd_rel13.tl.tag);
	IN_OUT_ASSERT(nb_harq_indication_body.nb_harq_pdu_list[0].ul_cqi_information.tl.tag);
	
}

void nfapi_test_nrach_indication()
{
	nfapi_nrach_indication_t in;
	memset(&in, 0, sizeof(in));
	nfapi_nrach_indication_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_NRACH_INDICATION;
	in.header.message_length = 0;

	in.sfn_sf = 1234;
	
	in.nrach_indication_body.tl.tag = NFAPI_NRACH_INDICATION_BODY_TAG;
	in.nrach_indication_body.number_of_initial_scs_detected = 1;
	
	nfapi_nrach_indication_pdu_t nrach_pdu[1];
	in.nrach_indication_body.nrach_pdu_list = &nrach_pdu[0];
	
	in.nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.tl.tag = NFAPI_NRACH_INDICATION_REL13_TAG;
	
	


	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	
	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	IN_OUT_ASSERT(sfn_sf);
	IN_OUT_ASSERT(nrach_indication_body.tl.tag);
	IN_OUT_ASSERT(nrach_indication_body.number_of_initial_scs_detected);
	IN_OUT_ASSERT(nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.tl.tag);
	IN_OUT_ASSERT(nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.rnti);
	IN_OUT_ASSERT(nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.initial_sc);
	IN_OUT_ASSERT(nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.timing_advance);
	IN_OUT_ASSERT(nrach_indication_body.nrach_pdu_list[0].nrach_indication_rel13.nrach_ce_level);	
	
}

void nfapi_test_dl_node_sync()
{
	nfapi_dl_node_sync_t in;
	nfapi_dl_node_sync_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_DL_NODE_SYNC;
	in.header.message_length = 0;

	in.t1 = 10239999;
	in.delta_sfn_sf = -987;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.t1, out.t1);
	CU_ASSERT_EQUAL(in.delta_sfn_sf, out.delta_sfn_sf);
}
void nfapi_test_ul_node_sync()
{
	nfapi_ul_node_sync_t in;
	nfapi_ul_node_sync_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_UL_NODE_SYNC;
	in.header.message_length = 0;

	in.t1 = 10239999;
	in.t2 = 10239999;
	in.t2 = 10239999;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);
	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);
	
	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.t1, out.t1);
	CU_ASSERT_EQUAL(in.t2, out.t2);
	CU_ASSERT_EQUAL(in.t3, out.t3);
}
void nfapi_test_timing_info()
{
	nfapi_timing_info_t in;
	nfapi_timing_info_t out;

	in.header.phy_id = NFAPI_PHY_ID_NA;
	in.header.message_id = NFAPI_TIMING_INFO;
	in.header.message_length = 0;
	in.header.m_segment_sequence = 0xDDDD;
	in.header.checksum = 0xEEEEEEEE;

	in.last_sfn_sf = 0xDEAD; //10239999;
	in.time_since_last_timing_info = 4294967295u;
	in.dl_config_jitter = 4294967295u;
	in.tx_request_jitter = 4294967295u;
	in.ul_config_jitter = 4294967295u;
	in.hi_dci0_jitter = 4294967295u;
	in.dl_config_latest_delay = 0; //-2147483648L;
	in.tx_request_latest_delay = 2147483647u;
	in.ul_config_latest_delay = 0; //-2147483648L ;
	in.hi_dci0_latest_delay =  0; //2147483647u;
	in.dl_config_earliest_arrival = 0; //-2147483648L;
	in.tx_request_earliest_arrival = 0; //2147483647u;
	in.ul_config_earliest_arrival = 0; //2147483648L;
	in.hi_dci0_earliest_arrival = -0; //2147483647u;

	int packedMessageLength = nfapi_p7_message_pack(&in, gTestNfapiMessageTx, MAX_PACKED_MESSAGE_SIZE, 0);

	nfapi_p7_message_unpack(gTestNfapiMessageTx, packedMessageLength, &out, sizeof(out), 0);

	CU_ASSERT_EQUAL(in.header.message_id, out.header.message_id);
	CU_ASSERT_EQUAL(in.last_sfn_sf, out.last_sfn_sf);
	CU_ASSERT_EQUAL(in.time_since_last_timing_info, out.time_since_last_timing_info);
	CU_ASSERT_EQUAL(in.dl_config_jitter, out.dl_config_jitter);
	CU_ASSERT_EQUAL(in.tx_request_jitter, out.tx_request_jitter);
	CU_ASSERT_EQUAL(in.ul_config_jitter, out.ul_config_jitter);
	CU_ASSERT_EQUAL(in.hi_dci0_jitter, out.hi_dci0_jitter);
	CU_ASSERT_EQUAL(in.dl_config_latest_delay, out.dl_config_latest_delay);
	CU_ASSERT_EQUAL(in.tx_request_latest_delay, out.tx_request_latest_delay);
	CU_ASSERT_EQUAL(in.ul_config_latest_delay, out.ul_config_latest_delay);
	CU_ASSERT_EQUAL(in.hi_dci0_latest_delay, out.hi_dci0_latest_delay);
	CU_ASSERT_EQUAL(in.dl_config_earliest_arrival, out.dl_config_earliest_arrival);
	CU_ASSERT_EQUAL(in.tx_request_earliest_arrival, out.tx_request_earliest_arrival);
	CU_ASSERT_EQUAL(in.ul_config_earliest_arrival, out.ul_config_earliest_arrival);
	CU_ASSERT_EQUAL(in.hi_dci0_earliest_arrival, out.hi_dci0_earliest_arrival);
}

void nfapi_struct_sizes()
{

	
	printf("P5\n");
	printf("nfapi_pnf_param_request %zu\n", sizeof(nfapi_pnf_param_request_t));
	printf("nfapi_pnf_param_response %zu\n", sizeof(nfapi_pnf_param_response_t));
	printf("nfapi_pnf_config_request %zu\n", sizeof(nfapi_pnf_config_request_t));
	printf("nfapi_pnf_config_response %zu\n", sizeof(nfapi_pnf_config_response_t));
	printf("nfapi_pnf_start_request %zu\n", sizeof(nfapi_pnf_start_request_t));
	printf("nfapi_pnf_start_response %zu\n", sizeof(nfapi_pnf_start_response_t));
	printf("nfapi_pnf_stop_request %zu\n", sizeof(nfapi_pnf_stop_request_t));
	printf("nfapi_pnf_stop_request %zu\n", sizeof(nfapi_pnf_stop_response_t));
	printf("nfapi_param_request %zu\n", sizeof(nfapi_param_request_t));
	printf("nfapi_param_response %zu\n", sizeof(nfapi_param_response_t));
	printf("nfapi_config_request %zu\n", sizeof(nfapi_config_request_t));
	printf("nfapi_config_response %zu\n", sizeof(nfapi_config_response_t));
	printf("nfapi_start_request %zu\n", sizeof(nfapi_start_request_t));
	printf("nfapi_start_response %zu\n", sizeof(nfapi_start_response_t));
	printf("nfapi_stop_request %zu\n", sizeof(nfapi_stop_request_t));
	printf("nfapi_stop_response %zu\n", sizeof(nfapi_stop_response_t));
	printf("nfapi_measurement_request %zu\n", sizeof(nfapi_measurement_request_t));
	printf("nfapi_measurement_response %zu\n", sizeof(nfapi_measurement_response_t));

	printf("P7\n");
	printf("nfapi_timing_info_t %zu\n", sizeof(nfapi_timing_info_t));
	printf("nfapi_dl_config_request_t %zu\n", sizeof(nfapi_dl_config_request_t));
	printf("nfapi_ul_config_request_t %zu\n", sizeof(nfapi_ul_config_request_t));
	printf("nfapi_hi_dci0_request_t %zu\n", sizeof(nfapi_hi_dci0_request_t));
	printf("nfapi_tx_request_t %zu\n", sizeof(nfapi_tx_request_t));
	printf("nfapi_crc_indication_t %zu\n", sizeof(nfapi_crc_indication_t));
	printf("nfapi_sr_indication_t %zu\n", sizeof(nfapi_sr_indication_t));
	printf("nfapi_srs_indication_t %zu\n", sizeof(nfapi_srs_indication_t));
	printf("nfapi_harq_indication_t %zu\n", sizeof(nfapi_harq_indication_t));
	printf("nfapi_rx_indication_t %zu\n", sizeof(nfapi_rx_indication_t));
	printf("nfapi_rach_indication_t %zu\n", sizeof(nfapi_rach_indication_t));
	printf("nfapi_cqi_indication_t %zu\n", sizeof(nfapi_cqi_indication_t));
	printf("nfapi_nb_harq_indication_t %zu\n", sizeof(nfapi_nb_harq_indication_t));
	printf("nfapi_nrach_indication_t %zu\n", sizeof(nfapi_nrach_indication_t));
	printf("nfapi_lbt_dl_config_request_t %zu\n", sizeof(nfapi_lbt_dl_config_request_t));
	printf("nfapi_lbt_dl_indication_t %zu\n", sizeof(nfapi_lbt_dl_indication_t));

	printf("P4\n");
	printf("nfapi_lte_rssi_request_t %zu\n", sizeof(nfapi_lte_rssi_request_t));
	printf("nfapi_utran_rssi_request_t %zu\n", sizeof(nfapi_utran_rssi_request_t));
	printf("nfapi_geran_rssi_request_t %zu\n", sizeof(nfapi_geran_rssi_request_t));
	printf("nfapi_lte_cell_search_request_t %zu\n", sizeof(nfapi_lte_cell_search_request_t));
	printf("nfapi_utran_cell_search_request_t %zu\n", sizeof(nfapi_utran_cell_search_request_t));
	printf("nfapi_geran_cell_search_request_t %zu\n", sizeof(nfapi_geran_cell_search_request_t));
	printf("nfapi_lte_cell_search_indication_t %zu\n", sizeof(nfapi_lte_cell_search_indication_t));
	printf("nfapi_utran_cell_search_indication_t %zu\n", sizeof(nfapi_utran_cell_search_indication_t));
	printf("nfapi_geran_cell_search_indication_t %zu\n", sizeof(nfapi_geran_cell_search_indication_t));
	printf("nfapi_lte_broadcast_detect_request_t %zu\n", sizeof(nfapi_lte_broadcast_detect_request_t));
	printf("nfapi_utran_broadcast_detect_request_t %zu\n", sizeof(nfapi_utran_broadcast_detect_request_t));
	printf("nfapi_lte_broadcast_detect_indication_t %zu\n", sizeof(nfapi_lte_broadcast_detect_indication_t));
	printf("nfapi_utran_broadcast_detect_indication_t %zu\n", sizeof(nfapi_utran_broadcast_detect_indication_t));
	printf("nfapi_lte_system_information_schedule_request_t %zu\n", sizeof(nfapi_lte_system_information_schedule_request_t));
	printf("nfapi_lte_system_information_request_t %zu\n", sizeof(nfapi_lte_system_information_request_t));
	printf("nfapi_utran_system_information_request_t %zu\n", sizeof(nfapi_utran_system_information_request_t));
	printf("nfapi_geran_system_information_request_t %zu\n", sizeof(nfapi_geran_system_information_request_t));
	printf("nfapi_lte_system_information_indication_t %zu\n", sizeof(nfapi_lte_system_information_indication_t));
	printf("nfapi_utran_system_information_indication_t %zu\n", sizeof(nfapi_utran_system_information_indication_t));
	printf("nfapi_geran_system_information_indication_t %zu\n", sizeof(nfapi_geran_system_information_indication_t));
}

/************* Test Runner Code goes here **************/


int main ( int argc, char** argv)
{

/*
        printf("1..3\n");
        printf("ok 1 - run good\n");	
        printf("ok 2 - mojojojo\n");	
        printf("not ok 3 - not implemented\n");	
	return 0;
	*/


	int i;
	printf("%d \n", argc);
	for(i = 0; i < argc; ++i)
	{
		if(argv[i] != 0)
			printf("%s \n", argv[i]);
	}

	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite( "nfapi_test_suite", init_suite, clean_suite );
	if ( NULL == pSuite ) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	//if ( (NULL == CU_add_test(pSuite, "nfapi_test_pnf_config_req", nfapi_test_pnf_config_req)) ||
	//     (NULL == CU_add_test(pSuite, "nfapi_test_2", nfapi_test_2)) 
	//   )
	// {
	//   CU_cleanup_registry();
	//   return CU_get_error();
	//}


	CU_pSuite pSuiteP4 = CU_add_suite( "nfapi_p4_pack_unpack_test_suite", init_suite, clean_suite );
	if(pSuiteP4)
	{
		if((NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_lte", nfapi_test_rssi_request_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_lte2", nfapi_test_rssi_request_lte2)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_lte_overrun", nfapi_test_rssi_request_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_lte_rat_type_mismatch", nfapi_test_rssi_request_lte_rat_type_mismatch)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_utran", nfapi_test_rssi_request_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_geran", nfapi_test_rssi_request_geran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_request_nb_iot", nfapi_test_rssi_request_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_response", nfapi_test_rssi_response)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_indication", nfapi_test_rssi_indication)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_rssi_indication_overrun", nfapi_test_rssi_indication_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_lte", nfapi_test_cell_search_request_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_lte_overrun", nfapi_test_cell_search_request_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_utran", nfapi_test_cell_search_request_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_utran_overrun", nfapi_test_cell_search_request_utran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_geran", nfapi_test_cell_search_request_geran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_geran_overrun", nfapi_test_cell_search_request_geran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_request_nb_iot", nfapi_test_cell_search_request_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_response", nfapi_test_cell_search_response)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_lte", nfapi_test_cell_search_indication_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_lte_overrun", nfapi_test_cell_search_indication_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_utran", nfapi_test_cell_search_indication_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_utran_overrun", nfapi_test_cell_search_indication_utran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_geran", nfapi_test_cell_search_indication_geran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_geran_overrun", nfapi_test_cell_search_indication_geran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_state_overrun", nfapi_test_cell_search_indication_state_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_cell_search_indication_nb_iot", nfapi_test_cell_search_indication_nb_iot)) ||				
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_request_lte", nfapi_test_broadcast_detect_request_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_request_utran", nfapi_test_broadcast_detect_request_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_request_nb_iot", nfapi_test_broadcast_detect_request_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_request_state_overrun", nfapi_test_broadcast_detect_request_state_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_response", nfapi_test_broadcast_detect_response)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_lte", nfapi_test_broadcast_detect_indication_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_lte_overrun", nfapi_test_broadcast_detect_indication_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_utran", nfapi_test_broadcast_detect_indication_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_utran_overrun", nfapi_test_broadcast_detect_indication_utran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_state_overrun", nfapi_test_broadcast_detect_indication_state_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_broadcast_detect_indication_nb_iot", nfapi_test_broadcast_detect_indication_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_request_lte", nfapi_test_system_information_schedule_request_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_request_state_overrun", nfapi_test_system_information_schedule_request_state_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_request_nb_iot", nfapi_test_system_information_schedule_request_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_response", nfapi_test_system_information_schedule_response)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_indication_lte", nfapi_test_system_information_schedule_indication_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_schedule_indication_nb_iot", nfapi_test_system_information_schedule_indication_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_lte", nfapi_test_system_information_request_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_lte_overrun", nfapi_test_system_information_request_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_utran", nfapi_test_system_information_request_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_geran", nfapi_test_system_information_request_geran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_state_overrun", nfapi_test_system_information_request_state_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_request_nb_iot", nfapi_test_system_information_request_nb_iot)) ||				
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_response", nfapi_test_system_information_response)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_lte", nfapi_test_system_information_indication_lte)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_lte_overrun", nfapi_test_system_information_indication_lte_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_utran", nfapi_test_system_information_indication_utran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_utran_overrun", nfapi_test_system_information_indication_utran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_geran", nfapi_test_system_information_indication_geran)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_geran_overrun", nfapi_test_system_information_indication_geran_overrun)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_system_information_indication_nb_iot", nfapi_test_system_information_indication_nb_iot)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_nmm_stop_request", nfapi_test_nmm_stop_request)) ||
				(NULL == CU_add_test(pSuiteP4, "nfapi_test_nmm_stop_response", nfapi_test_nmm_stop_response))
				) 
				{
					CU_cleanup_registry();
					return CU_get_error();
				}
	}
	else
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_pSuite pSuiteP5 = CU_add_suite( "nfapi_p5_pack_unpack_test_suite", init_suite, clean_suite );
	if(pSuiteP5)
	{
		if((NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_param_request", nfapi_test_pnf_param_request)) ||
		    (NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_param_request_ve", nfapi_test_pnf_param_request_ve)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_param_response", nfapi_test_pnf_param_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_config_request", nfapi_test_pnf_config_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_config_response", nfapi_test_pnf_config_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_config_response1", nfapi_test_pnf_config_response1)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_start_request", nfapi_test_pnf_start_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_start_response", nfapi_test_pnf_start_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_stop_request", nfapi_test_pnf_stop_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_pnf_stop_request", nfapi_test_pnf_stop_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_param_request", nfapi_test_param_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_param_response", nfapi_test_param_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_config_request", nfapi_test_config_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_config_response", nfapi_test_config_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_start_request", nfapi_test_start_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_start_response", nfapi_test_start_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_stop_request", nfapi_test_stop_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_stop_response", nfapi_test_stop_response)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_measurement_request", nfapi_test_measurement_request)) ||
			(NULL == CU_add_test(pSuiteP5, "nfapi_test_measurement_response", nfapi_test_measurement_response))
		  )
		{
			CU_cleanup_registry();
			return CU_get_error();
		}

	}
	else
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_pSuite pSuiteP7 = CU_add_suite( "nfapi_p7_pack_unpack_test_suite", init_suite, clean_suite );
	if(pSuiteP7)
	{
		if((NULL == CU_add_test(pSuiteP7, "nfapi_test_dl_config_request", nfapi_test_dl_config_request)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_ul_config_request", nfapi_test_ul_config_request)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_hi_dci0_request", nfapi_test_hi_dci0_request)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_tx_request", nfapi_test_tx_request)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_harq_indication", nfapi_test_harq_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_crc_indication", nfapi_test_crc_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_rx_ulsch_indication", nfapi_test_rx_ulsch_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_rach_indication", nfapi_test_rach_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_srs_indication", nfapi_test_srs_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_rx_sr_indication", nfapi_test_rx_sr_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_rx_cqi_indication", nfapi_test_rx_cqi_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_lbt_dl_config_request", nfapi_test_lbt_dl_config_request)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_lbt_dl_indication", nfapi_test_lbt_dl_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_lbt_dl_indication_invalid_pdu_type", nfapi_test_lbt_dl_indication_invalid_pdu_type)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_nb_harq_indication", nfapi_test_nb_harq_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_nrach_indication", nfapi_test_nrach_indication)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_dl_node_sync", nfapi_test_dl_node_sync)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_ul_node_sync", nfapi_test_ul_node_sync)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_test_timing_info", nfapi_test_timing_info)) ||
			(NULL == CU_add_test(pSuiteP7, "nfapi_struct_sizes", nfapi_struct_sizes))
		  )
		{
			CU_cleanup_registry();
			return CU_get_error();
		}

	}
	else
	{
		CU_cleanup_registry();
		return CU_get_error();
	}


	// Run all tests using the basic interface
	CU_basic_set_mode(CU_BRM_VERBOSE);
	//printf(" CU_basic_set_mode set \n");
	CU_set_output_filename("nfapi_unit_test_results.xml");

	CU_basic_run_tests();
	//CU_automated_run_tests();

	//CU_set_test_complete_handler(automated_test_complete_message_handler);
	//CU_run_all_tests();

	
	CU_pSuite s = CU_get_registry()->pSuite;
	int count = 0;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

	printf("%d..%d\n", 1, count);



	s = CU_get_registry()->pSuite;
	count = 1;
	while(s)
	{
		CU_pTest t = s->pTest;
		while(t)
		{
			int pass = 1;
			CU_FailureRecord* failures = CU_get_failure_list();
			while(failures)
			{
				if(strcmp(failures->pSuite->pName, s->pName) == 0 &&
				   strcmp(failures->pTest->pName, t->pName) == 0)
				{
					pass = 0;
					failures = 0;
				}
				else
				{
					failures = failures->pNext;
				}
			}

			if(pass)
				printf("ok %d - %s:%s\n", count, s->pName, t->pName);
			else 
				printf("not ok %d - %s:%s\n", count, s->pName, t->pName);

			count++;
			t = t->pNext;
		}
		s = s->pNext;
	}

	/*
	s = CU_get_registry()->pSuite;
	while(s)
	{
	   printf("Suite %s\n", s->pName);
	   CU_pTest t = s->pTest;
	   while(t)
	   {
		   printf(" Test %s\n", t->pName);
			i//CU_ErrorCode e = CU_basic_run_test(s, t);

		   t = t->pNext;
	   }
	   s = s->pNext;
	}
	*/



	//printf("CU_basic_run_tests completed \n");
	//CU_basic_show_failures(CU_get_failure_list());
	//printf("CU_basic_show_failures completed\n\n");
	/*
	// Run all tests using the automated interface
	CU_automated_run_tests();
	CU_list_tests_to_file();

	// Run all tests using the console interface
	CU_console_run_tests();
	*/
	/* Clean up registry and return */


	CU_cleanup_registry();
	return CU_get_error();

}
