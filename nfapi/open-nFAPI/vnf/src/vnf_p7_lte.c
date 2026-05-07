/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "vnf_p7.h"

#define SYNC_CYCLE_COUNT 2

uint16_t increment_sfn_sf(uint16_t sfn_sf)
{
	if((sfn_sf & 0xF) == 9)
	{
		sfn_sf += 0x0010;
		sfn_sf &= 0x3FF0;
	}
	else if((sfn_sf & 0xF) > 9)
	{
		// error should not happen
	}
	else
	{
		sfn_sf++;
	}

	return sfn_sf;
}

static uint32_t get_sf_time(uint32_t now_hr, uint32_t sf_start_hr)
{
	if(now_hr < sf_start_hr)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "now is earlier than start of subframe\n");
		return 0;
	}
	else
	{
		uint32_t now_us = TIMEHR_USEC(now_hr);
		uint32_t sf_start_us = TIMEHR_USEC(sf_start_hr);

		// if the us have wrapped adjust for it
		if(now_hr < sf_start_us)
		{
			now_us += 1000000;
		}

		return now_us - sf_start_us;
	}
}

uint32_t calculate_t1(uint16_t sfn_sf, uint32_t sf_start_time_hr)
{
	uint32_t now_time_hr = vnf_get_current_time_hr();

	uint32_t sf_time_us = get_sf_time(now_time_hr, sf_start_time_hr);

	uint32_t t1 = (NFAPI_SFNSF2DEC(sfn_sf) * 1000) + sf_time_us;

	return t1;
}

uint32_t calculate_t4(uint32_t now_time_hr, uint16_t sfn_sf, uint32_t sf_start_time_hr)
{
	uint32_t sf_time_us = get_sf_time(now_time_hr, sf_start_time_hr);

	uint32_t t4 = (NFAPI_SFNSF2DEC(sfn_sf) * 1000) + sf_time_us;

	return t4;

}

uint16_t increment_sfn_sf_by(uint16_t sfn_sf, uint8_t increment)
{
	while(increment > 0)
	{
		sfn_sf = increment_sfn_sf(sfn_sf);
		--increment;
	}

	return sfn_sf;
}

int send_mac_subframe_indications(vnf_p7_t* vnf_p7)
{
	nfapi_vnf_p7_connection_info_t* curr = vnf_p7->p7_connections;
	while(curr != 0)
	{
		if(curr->in_sync == 1)
		{
            // suggestion fix by Haruki NAOI
			vnf_p7->_public.subframe_indication(&(vnf_p7->_public), curr->phy_id, curr->sfn_sf);
		}

		curr = curr->next;
	}

	return 0;
}

int vnf_build_send_dl_node_sync(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info)
{
	nfapi_dl_node_sync_t dl_node_sync;
	memset(&dl_node_sync, 0, sizeof(dl_node_sync));

	dl_node_sync.header.phy_id = p7_info->phy_id;
	dl_node_sync.header.message_id = NFAPI_DL_NODE_SYNC;
	dl_node_sync.t1 = calculate_t1(p7_info->sfn_sf, vnf_p7->sf_start_time_hr);
	dl_node_sync.delta_sfn_sf = 0;

	return vnf_p7_pack_and_send_p7_msg(vnf_p7, &dl_node_sync.header);
}

int vnf_sync(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info)
{

	if(p7_info->in_sync == 1)
	{
		uint16_t dl_sync_period_mask = p7_info->dl_in_sync_period-1;
		uint16_t sfn_sf_dec = NFAPI_SFNSF2DEC(p7_info->sfn_sf);

		if ((((sfn_sf_dec + p7_info->dl_in_sync_offset) % NFAPI_MAX_SFNSFDEC) & dl_sync_period_mask) == 0)
		{
			vnf_build_send_dl_node_sync(vnf_p7, p7_info);
		}
	}
	else
	{
		uint16_t dl_sync_period_mask = p7_info->dl_out_sync_period-1;
		uint16_t sfn_sf_dec = NFAPI_SFNSF2DEC(p7_info->sfn_sf);

		if ((((sfn_sf_dec + p7_info->dl_out_sync_offset) % NFAPI_MAX_SFNSFDEC) & dl_sync_period_mask) == 0)
		{
			vnf_build_send_dl_node_sync(vnf_p7, p7_info);
		}
	}
	return 0;
}

void vnf_handle_harq_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_harq_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.harq_indication)
			{
				(vnf_p7->_public.harq_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.harq_indication_body.harq_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_crc_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_crc_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.crc_indication)
			{
				(vnf_p7->_public.crc_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.crc_indication_body.crc_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_rx_ulsch_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_rx_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.rx_indication)
			{
				(vnf_p7->_public.rx_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.rx_indication_body.rx_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}

}

void vnf_handle_rach_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_rach_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.rach_indication)
			{
				(vnf_p7->_public.rach_indication)(&vnf_p7->_public, &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.rach_indication_body.preamble_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);

	}
}

void vnf_handle_srs_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_srs_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.srs_indication)
			{
				(vnf_p7->_public.srs_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.srs_indication_body.srs_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_rx_sr_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_sr_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.sr_indication)
			{
				(vnf_p7->_public.sr_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.sr_indication_body.sr_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_rx_cqi_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_cqi_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.cqi_indication)
			{
				(vnf_p7->_public.cqi_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.cqi_indication_body.cqi_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.cqi_indication_body.cqi_raw_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);

	}

}

void vnf_handle_lbt_dl_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_lbt_dl_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.lbt_dl_indication)
			{
				(vnf_p7->_public.lbt_dl_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.lbt_dl_indication_body.lbt_indication_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_nb_harq_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nb_harq_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.nb_harq_indication)
			{
				(vnf_p7->_public.nb_harq_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.nb_harq_indication_body.nb_harq_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_nrach_indication(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_nrach_indication_t ind;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(ind), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}
		else
		{
			if(vnf_p7->_public.nrach_indication)
			{
				(vnf_p7->_public.nrach_indication)(&(vnf_p7->_public), &ind);
			}
		}

		vnf_p7_codec_free(vnf_p7, ind.nrach_indication_body.nrach_pdu_list);
		vnf_p7_codec_free(vnf_p7, ind.vendor_extension);
	}
}

void vnf_handle_ue_release_resp(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{

	// ensure it's valid
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else
	{
		nfapi_ue_release_response_t resp;

		if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &resp, sizeof(resp), &vnf_p7->_public.codec_config) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: Failed to unpack message\n", __FUNCTION__);
		}


		vnf_p7_codec_free(vnf_p7, resp.vendor_extension);
	}
}

void vnf_handle_p7_vendor_extension(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7, uint16_t message_id)
{
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: NULL parameters\n", __FUNCTION__);
	}
	else if(vnf_p7->_public.allocate_p7_vendor_ext)
	{
		uint16_t msg_size;
		nfapi_p7_message_header_t* msg = vnf_p7->_public.allocate_p7_vendor_ext(message_id, &msg_size);

		if(msg == 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "%s failed to allocate vendor extention structure\n", __FUNCTION__);
			return;
		}

		int unpack_result = nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, msg, msg_size, &vnf_p7->_public.codec_config);

		if(unpack_result == 0)
		{
			if(vnf_p7->_public.vendor_ext)
				vnf_p7->_public.vendor_ext(&(vnf_p7->_public), msg);
		}

		if(vnf_p7->_public.deallocate_p7_vendor_ext)
			vnf_p7->_public.deallocate_p7_vendor_ext(msg);

	}
}

void vnf_handle_ul_node_sync(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	uint32_t now_time_hr = vnf_get_current_time_hr();

	if (pRecvMsg == NULL || vnf_p7  == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_ul_node_sync: NULL parameters\n");
		return;
	}

	nfapi_ul_node_sync_t ind;
	if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(nfapi_ul_node_sync_t), &vnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack ul_node_sync\n");
		return;
	}

	nfapi_vnf_p7_connection_info_t* phy = vnf_p7_connection_info_list_find(vnf_p7, ind.header.phy_id);
	uint32_t t4 = calculate_t4(now_time_hr, phy->sfn_sf, vnf_p7->sf_start_time_hr);

	uint32_t tx_2_rx = t4>ind.t1 ? t4 - ind.t1 : t4 + NFAPI_MAX_SFNSFDEC - ind.t1 ;
	uint32_t pnf_proc_time = ind.t3 - ind.t2;

	// divide by 2 using shift operator
	uint32_t latency =  (tx_2_rx - pnf_proc_time) >> 1;

	if(!(phy->filtered_adjust))
	{
		phy->latency[phy->min_sync_cycle_count] = latency;

		NFAPI_TRACE(NFAPI_TRACE_NOTE, "(%4d/%d) PNF to VNF !sync phy_id:%d (t1/2/3/4:%8u, %8u, %8u, %8u) txrx:%4u procT:%3u latency(us):%4d\n",
				NFAPI_SFNSF2SFN(phy->sfn_sf), NFAPI_SFNSF2SF(phy->sfn_sf), ind.header.phy_id, ind.t1, ind.t2, ind.t3, t4,
				tx_2_rx, pnf_proc_time, latency);
	}
	else
	{
		phy->latency[phy->min_sync_cycle_count] = latency;

		{
			if (ind.t2 < phy->previous_t2 && ind.t1 > phy->previous_t1)
			{
				// Only t2 wrap has occurred!!!
				phy->sf_offset = (NFAPI_MAX_SFNSFDEC + ind.t2) - ind.t1 - latency;
			}
			else if (ind.t2 > phy->previous_t2 && ind.t1 < phy->previous_t1)
			{
				// Only t1 wrap has occurred
				phy->sf_offset = ind.t2 - ( ind.t1 + NFAPI_MAX_SFNSFDEC) - latency;
			}
			else
			{
				// Either no wrap or both have wrapped
				phy->sf_offset = ind.t2 - ind.t1 - latency;
			}

			if (phy->sf_offset_filtered == 0)
			{
				phy->sf_offset_filtered = phy->sf_offset;
			}
			else
			{
				int32_t oldFilteredValueShifted = phy->sf_offset_filtered << 5;
				int32_t newOffsetShifted = phy->sf_offset << 5;

				// 1/8 of new and 7/8 of old
				phy->sf_offset_filtered = ((newOffsetShifted >> 3) + ((oldFilteredValueShifted * 7) >> 3)) >> 5;
			}
		}

		if(1)
		{
                  struct timespec ts;
                  clock_gettime(CLOCK_MONOTONIC, &ts);

			NFAPI_TRACE(NFAPI_TRACE_INFO, "(%4d/%1d) %ld.%ld PNF to VNF phy_id:%2d (t1/2/3/4:%8u, %8u, %8u, %8u) txrx:%4u procT:%3u latency(us):%4d(avg:%4d) offset(us):%8d filtered(us):%8d wrap[t1:%u t2:%u]\n",
					NFAPI_SFNSF2SFN(phy->sfn_sf), NFAPI_SFNSF2SF(phy->sfn_sf), ts.tv_sec, ts.tv_nsec, ind.header.phy_id,
					ind.t1, ind.t2, ind.t3, t4,
					tx_2_rx, pnf_proc_time, latency, phy->average_latency, phy->sf_offset, phy->sf_offset_filtered,
					(ind.t1<phy->previous_t1), (ind.t2<phy->previous_t2));
		}

	}

        if (phy->filtered_adjust && (phy->sf_offset_filtered > 1e6 || phy->sf_offset_filtered < -1e6))
        {
          phy->filtered_adjust = 0;
          phy->zero_count=0;
          phy->min_sync_cycle_count = 2;
          phy->in_sync = 0;
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s - ADJUST TOO BAD - go out of filtered phy->sf_offset_filtered:%d\n", __FUNCTION__, phy->sf_offset_filtered);
        }

	if(phy->min_sync_cycle_count)
		phy->min_sync_cycle_count--;

	if(phy->min_sync_cycle_count == 0)
	{
		uint32_t curr_sfn_sf = phy->sfn_sf;
		int32_t sfn_sf_dec = NFAPI_SFNSF2DEC(phy->sfn_sf);

		if(!phy->filtered_adjust)
		{
			int i = 0;
			for(i = 0; i < SYNC_CYCLE_COUNT; ++i)
			{
				phy->average_latency += phy->latency[i];

			}
			phy->average_latency /= SYNC_CYCLE_COUNT;

			phy->sf_offset = ind.t2 - (ind.t1 - phy->average_latency);

			sfn_sf_dec += (phy->sf_offset / 1000);
		}
		else
		{
			sfn_sf_dec += ((phy->sf_offset_filtered + 500) / 1000);	//Round up go from microsecond to subframe(1ms)
		}

		if(sfn_sf_dec < 0)
		{
			sfn_sf_dec += NFAPI_MAX_SFNSFDEC;
		}
		else if( sfn_sf_dec >= NFAPI_MAX_SFNSFDEC)
		{
			sfn_sf_dec -= NFAPI_MAX_SFNSFDEC;
		}

		uint16_t new_sfn_sf = NFAPI_SFNSFDEC2SFNSF(sfn_sf_dec);


		{
			phy->adjustment = NFAPI_SFNSF2DEC(new_sfn_sf) - NFAPI_SFNSF2DEC(curr_sfn_sf);

			NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF to VNF phy_id:%d adjustment%d phy->previous_sf_offset_filtered:%d phy->previous_sf_offset_filtered:%d phy->sf_offset_trend:%d\n", ind.header.phy_id, phy->adjustment, phy->previous_sf_offset_filtered, phy->previous_sf_offset_filtered, phy->sf_offset_trend);

			phy->previous_t1 = 0;
			phy->previous_t2 = 0;

			if(phy->previous_sf_offset_filtered > 0)
			{
				if( phy->sf_offset_filtered > phy->previous_sf_offset_filtered)
				{
					// pnf is getting futher ahead of vnf
					phy->sf_offset_trend = (phy->sf_offset_filtered + phy->previous_sf_offset_filtered)/2;
				}
				else
				{
					// pnf is getting back in sync
				}
			}
			else if(phy->previous_sf_offset_filtered < 0)
			{
				if(phy->sf_offset_filtered < phy->previous_sf_offset_filtered)
				{
					// vnf is getting future ahead of pnf
					phy->sf_offset_trend = (-(phy->sf_offset_filtered + phy->previous_sf_offset_filtered)) /2;
				}
				else
				{
					//  vnf is getting back in sync
				}
			}


			int insync_minor_adjustment_1 = phy->sf_offset_trend / 6;
			int insync_minor_adjustment_2 = phy->sf_offset_trend / 2;


			if(insync_minor_adjustment_1 == 0)
				insync_minor_adjustment_1 = 2;

			if(insync_minor_adjustment_2 == 0)
				insync_minor_adjustment_2 = 10;

			if(!phy->filtered_adjust)
			{
				if(phy->adjustment < 10)
				{
					phy->zero_count++;

					if(phy->zero_count >= 10)
					{
						phy->filtered_adjust = 1;
						phy->zero_count = 0;

						NFAPI_TRACE(NFAPI_TRACE_NOTE, "***** Adjusting VNF SFN/SF switching to filtered mode\n");
					}
				}
				else
				{
					phy->zero_count = 0;
				}
			}
			else
			{
				// Fine level of adjustment
				if (phy->adjustment == 0)
				{
					if (phy->zero_count >= 10)
					{
						if(phy->in_sync == 0)
						{
							if(vnf_p7->_public.sync_indication)
								(vnf_p7->_public.sync_indication)(&(vnf_p7->_public), phy->in_sync);
						}

						phy->in_sync = 1;
					}
					else
					{
						phy->zero_count++;
					}

					if(phy->in_sync)
					{
						// in sync
						if(phy->sf_offset_filtered > 250)
						{
							// VNF is slow
							phy->insync_minor_adjustment = insync_minor_adjustment_1; //25;
							phy->insync_minor_adjustment_duration = ((phy->sf_offset_filtered) / insync_minor_adjustment_1);
						}
						else if(phy->sf_offset_filtered < -250)
						{
							// VNF is fast
							phy->insync_minor_adjustment = -(insync_minor_adjustment_1); //25;
							phy->insync_minor_adjustment_duration = (((phy->sf_offset_filtered) / -(insync_minor_adjustment_1)));
						}
						else
						{
							phy->insync_minor_adjustment = 0;
						}

						if(phy->insync_minor_adjustment != 0)
						{
							NFAPI_TRACE(NFAPI_TRACE_NOTE, "(%4d/%d) VNF phy_id:%d Apply minor insync adjustment %dus for %d subframes (sf_offset_filtered:%d) %d %d %d NEW:%d CURR:%d adjustment:%d\n",
										NFAPI_SFNSF2SFN(phy->sfn_sf), NFAPI_SFNSF2SF(phy->sfn_sf), ind.header.phy_id,
										phy->insync_minor_adjustment, phy->insync_minor_adjustment_duration,
                                                                                phy->sf_offset_filtered,
                                                                                insync_minor_adjustment_1, insync_minor_adjustment_2, phy->sf_offset_trend,
                                                                                NFAPI_SFNSF2DEC(new_sfn_sf),
                                                                                NFAPI_SFNSF2DEC(curr_sfn_sf),
                                                                                phy->adjustment);
						}
					}
				}
				else
				{
					if (phy->in_sync)
					{
						if(phy->adjustment == 0)
						{
						}
						else if(phy->adjustment > 0)
						{
							// VNF is slow
							{
								if(phy->sf_offset_filtered > 250)
								{
									// VNF is slow
									phy->insync_minor_adjustment = insync_minor_adjustment_2;
									phy->insync_minor_adjustment_duration = 2 * ((phy->sf_offset_filtered - 250) / insync_minor_adjustment_2);
								}
								else if(phy->sf_offset_filtered < -250)
								{
									// VNF is fast
									phy->insync_minor_adjustment = -(insync_minor_adjustment_2);
									phy->insync_minor_adjustment_duration = 2 * ((phy->sf_offset_filtered + 250) / -(insync_minor_adjustment_2));
								}

							}

							NFAPI_TRACE(NFAPI_TRACE_NOTE, "(%4d/%d) VNF phy_id:%d Apply minor insync adjustment %dus for %d subframes (adjustment:%d sf_offset_filtered:%d) %d %d %d NEW:%d CURR:%d adj:%d\n",
										NFAPI_SFNSF2SFN(phy->sfn_sf), NFAPI_SFNSF2SF(phy->sfn_sf), ind.header.phy_id,
										phy->insync_minor_adjustment, phy->insync_minor_adjustment_duration, phy->adjustment, phy->sf_offset_filtered,
										insync_minor_adjustment_1, insync_minor_adjustment_2, phy->sf_offset_trend,
                                                                                NFAPI_SFNSF2DEC(new_sfn_sf),
                                                                                NFAPI_SFNSF2DEC(curr_sfn_sf),
                                                                                phy->adjustment);

						}
						else if(phy->adjustment < 0)
						{
							// VNF is fast
							{
								if(phy->sf_offset_filtered > 250)
								{
									// VNF is slow
									phy->insync_minor_adjustment = insync_minor_adjustment_2;
									phy->insync_minor_adjustment_duration = 2 * ((phy->sf_offset_filtered - 250) / insync_minor_adjustment_2);
								}
								else if(phy->sf_offset_filtered < -250)
								{
									// VNF is fast
									phy->insync_minor_adjustment = -(insync_minor_adjustment_2);
									phy->insync_minor_adjustment_duration = 2 * ((phy->sf_offset_filtered + 250) / -(insync_minor_adjustment_2));
								}
							}

							NFAPI_TRACE(NFAPI_TRACE_NOTE, "(%d/%d) VNF phy_id:%d Apply minor insync adjustment %dus for %d subframes (adjustment:%d sf_offset_filtered:%d) %d %d %d\n",
										NFAPI_SFNSF2SFN(phy->sfn_sf), NFAPI_SFNSF2SF(phy->sfn_sf), ind.header.phy_id,
										phy->insync_minor_adjustment, phy->insync_minor_adjustment_duration, phy->adjustment, phy->sf_offset_filtered,
										insync_minor_adjustment_1, insync_minor_adjustment_2, phy->sf_offset_trend);
						}
					}
				}
			}


			if(phy->in_sync == 0)
			{
				phy->sfn_sf = new_sfn_sf;
			}
		}

		// reset for next cycle
		phy->previous_sf_offset_filtered = phy->sf_offset_filtered;
		phy->min_sync_cycle_count = 2;
		phy->sf_offset_filtered = 0;
		phy->sf_offset = 0;
	}
	else
	{
		phy->previous_t1 = ind.t1;
		phy->previous_t2 = ind.t2;
	}
}

void vnf_handle_timing_info(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	if (pRecvMsg == NULL || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_timing_info: NULL parameters\n");
		return;
	}

	nfapi_timing_info_t ind;
	if(nfapi_p7_message_unpack(pRecvMsg, recvMsgLen, &ind, sizeof(nfapi_timing_info_t), &vnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack timing_info\n");
		return;
	}

        if (vnf_p7 && vnf_p7->p7_connections)
        {
          int16_t vnf_pnf_sfnsf_delta = NFAPI_SFNSF2DEC(vnf_p7->p7_connections[0].sfn_sf) - NFAPI_SFNSF2DEC(ind.last_sfn_sf);

          if (vnf_pnf_sfnsf_delta>0 || vnf_pnf_sfnsf_delta < 0)
          {
            NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() LARGE SFN/SF DELTA between PNF and VNF delta:%d VNF:%d PNF:%d\n\n\n\n\n\n\n\n\n", __FUNCTION__, vnf_pnf_sfnsf_delta, NFAPI_SFNSF2DEC(vnf_p7->p7_connections[0].sfn_sf), NFAPI_SFNSF2DEC(ind.last_sfn_sf));
            vnf_p7->p7_connections[0].sfn_sf = ind.last_sfn_sf;
          }
        }
}

static void vnf_dispatch_p7_message(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	nfapi_p7_message_header_t header;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s: invalid input params\n", __FUNCTION__);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &header, sizeof(header), &vnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	// ensure the message is sensible
	if (recvMsgLen < 8 || pRecvMsg == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
		return;
	}

	switch (header.message_id)
	{
		case NFAPI_UL_NODE_SYNC:
			vnf_handle_ul_node_sync(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_TIMING_INFO:
			vnf_handle_timing_info(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_HARQ_INDICATION:
			vnf_handle_harq_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_CRC_INDICATION:
			vnf_handle_crc_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_RX_ULSCH_INDICATION:
			vnf_handle_rx_ulsch_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_RACH_INDICATION:
			vnf_handle_rach_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_SRS_INDICATION:
			vnf_handle_srs_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_RX_SR_INDICATION:
			vnf_handle_rx_sr_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_RX_CQI_INDICATION:
			vnf_handle_rx_cqi_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_LBT_DL_INDICATION:
			vnf_handle_lbt_dl_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_NB_HARQ_INDICATION:
			vnf_handle_nb_harq_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_NRACH_INDICATION:
			vnf_handle_nrach_indication(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		case NFAPI_UE_RELEASE_RESPONSE:
			vnf_handle_ue_release_resp(pRecvMsg, recvMsgLen, vnf_p7);
			break;

		default:
			{
				if(header.message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
				   header.message_id <= NFAPI_VENDOR_EXT_MSG_MAX)
				{
					vnf_handle_p7_vendor_extension(pRecvMsg, recvMsgLen, vnf_p7, header.message_id);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Unknown message ID %d\n", header.message_id);
				}
			}
			break;
	}
}

static void vnf_handle_p7_message(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7)
{
	nfapi_p7_message_header_t messageHeader;

	// validate the input params
	if(pRecvMsg == NULL || recvMsgLen < 4 || vnf_p7 == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "vnf_handle_p7_message: invalid input params (%p %d %p)\n", pRecvMsg, recvMsgLen, vnf_p7);
		return;
	}

	// unpack the message header
	if (nfapi_p7_message_header_unpack(pRecvMsg, recvMsgLen, &messageHeader, sizeof(nfapi_p7_message_header_t), &vnf_p7->_public.codec_config) < 0)
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
		return;
	}

	if(vnf_p7->_public.checksum_enabled)
	{
		uint32_t checksum = nfapi_p7_calculate_checksum(pRecvMsg, recvMsgLen);
		if(checksum != messageHeader.checksum)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "Checksum verification failed %d %d msg:%d len:%d\n", checksum, messageHeader.checksum, messageHeader.message_id, recvMsgLen);
			return;
		}
	}

	uint8_t m = NFAPI_P7_GET_MORE(messageHeader.m_segment_sequence);
	uint8_t segment_num = NFAPI_P7_GET_SEGMENT(messageHeader.m_segment_sequence);
	uint8_t sequence_num = NFAPI_P7_GET_SEQUENCE(messageHeader.m_segment_sequence);


	if(m == 0 && segment_num == 0)
	{
		// we have a complete message
		// ensure the message is sensible
		if (recvMsgLen < 8 || pRecvMsg == NULL)
		{
			NFAPI_TRACE(NFAPI_TRACE_WARN, "Invalid message size: %d, ignoring\n", recvMsgLen);
			return;
		}

		vnf_dispatch_p7_message(pRecvMsg, recvMsgLen, vnf_p7);
	}
	else
	{
		nfapi_vnf_p7_connection_info_t* phy = vnf_p7_connection_info_list_find(vnf_p7, messageHeader.phy_id);

		if(phy)
		{
			vnf_p7_rx_message_t* rx_msg = vnf_p7_rx_reassembly_queue_add_segment(vnf_p7, &(phy->reassembly_queue), sequence_num, segment_num, m, pRecvMsg, recvMsgLen);

			if(rx_msg->num_segments_received == rx_msg->num_segments_expected)
			{
				// send the buffer on
				uint16_t i = 0;
				uint16_t length = 0;
				for(i = 0; i < rx_msg->num_segments_expected; ++i)
				{
					length += rx_msg->segments[i].length - (i > 0 ? NFAPI_P7_HEADER_LENGTH : 0);
				}

				if(phy->reassembly_buffer_size < length)
				{
					vnf_p7_free(vnf_p7, phy->reassembly_buffer);
					phy->reassembly_buffer = 0;
				}

				if(phy->reassembly_buffer == 0)
				{
					NFAPI_TRACE(NFAPI_TRACE_NOTE, "Resizing VNF_P7 Reassembly buffer %d->%d\n", phy->reassembly_buffer_size, length);
					phy->reassembly_buffer = (uint8_t*)vnf_p7_malloc(vnf_p7, length);

					if(phy->reassembly_buffer == 0)
					{
						NFAPI_TRACE(NFAPI_TRACE_NOTE, "Failed to allocate VNF_P7 reassemby buffer len:%d\n", length);
						return;
					}
                                       memset(phy->reassembly_buffer, 0, length);
					phy->reassembly_buffer_size = length;
				}

				uint16_t offset = 0;
				for(i = 0; i < rx_msg->num_segments_expected; ++i)
				{
					if(i == 0)
					{
						memcpy(phy->reassembly_buffer, rx_msg->segments[i].buffer, rx_msg->segments[i].length);
						offset += rx_msg->segments[i].length;
					}
					else
					{
						memcpy(phy->reassembly_buffer + offset, rx_msg->segments[i].buffer + NFAPI_P7_HEADER_LENGTH, rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH);
						offset += rx_msg->segments[i].length - NFAPI_P7_HEADER_LENGTH;
					}
				}

				vnf_dispatch_p7_message(phy->reassembly_buffer, length , vnf_p7);

				// delete the structure
				vnf_p7_rx_reassembly_queue_remove_msg(vnf_p7, &(phy->reassembly_queue), rx_msg);
			}

			vnf_p7_rx_reassembly_queue_remove_old_msgs(vnf_p7, &(phy->reassembly_queue), 1000);
		}
		else
		{

			NFAPI_TRACE(NFAPI_TRACE_INFO, "Unknown phy id %d\n", messageHeader.phy_id);
		}
	}
}

int vnf_p7_read_dispatch_message(vnf_p7_t* vnf_p7)
{
	int recvfrom_result = 0;
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_size = sizeof(remote_addr);

	do
	{
		// peek the header
		uint8_t header_buffer[NFAPI_P7_HEADER_LENGTH];
		recvfrom_result = recvfrom(vnf_p7->socket, header_buffer, NFAPI_P7_HEADER_LENGTH, MSG_DONTWAIT | MSG_PEEK, (struct sockaddr*)&remote_addr, &remote_addr_size);

		if(recvfrom_result > 0)
		{
			// get the segment size
			nfapi_p7_message_header_t header;
			if(nfapi_p7_message_header_unpack(header_buffer, NFAPI_P7_HEADER_LENGTH, &header, sizeof(header), 0) < 0)
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unpack message header failed, ignoring\n");
				return -1;
			}

			// resize the buffer if we have a large segment
			if(header.message_length > vnf_p7->rx_message_buffer_size)
			{
				NFAPI_TRACE(NFAPI_TRACE_NOTE, "reallocing rx buffer %d\n", header.message_length);
				vnf_p7->rx_message_buffer = realloc(vnf_p7->rx_message_buffer, header.message_length);
				vnf_p7->rx_message_buffer_size = header.message_length;
			}

			// read the segment
			recvfrom_result = recvfrom(vnf_p7->socket, vnf_p7->rx_message_buffer, header.message_length, MSG_WAITALL | MSG_TRUNC, (struct sockaddr*)&remote_addr, &remote_addr_size);
			NFAPI_TRACE(NFAPI_TRACE_INFO, "recvfrom_result = %d from %s():%d\n", recvfrom_result, __FUNCTION__, __LINE__);

			if (recvfrom_result > 0)
			{
				if (recvfrom_result != header.message_length)
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "(%d) Received unexpected number of bytes. %d != %d",
						    __LINE__, recvfrom_result, header.message_length);
					break;
				}
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Calling vnf_nr_reassemble_p7_message from %d\n", __LINE__);
				vnf_handle_p7_message(vnf_p7->rx_message_buffer, recvfrom_result, vnf_p7);
				return 0;
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_ERROR, "recvfrom failed %d %d\n", recvfrom_result, errno);
			}
		}

		if(recvfrom_result == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// return to the select
			}
			else
			{
				NFAPI_TRACE(NFAPI_TRACE_WARN, "%s recvfrom failed errno:%d\n", __FUNCTION__, errno);
			}
		}
	}
	while(recvfrom_result > 0);

	return 0;
}

void vnf_p7_release_msg(vnf_p7_t* vnf_p7, nfapi_p7_message_header_t* header)
{
	switch(header->message_id)
	{
		case NFAPI_HARQ_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_harq_indication_t*)(header))->harq_indication_body.harq_pdu_list);
			}
			break;
		case NFAPI_CRC_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_crc_indication_t*)(header))->crc_indication_body.crc_pdu_list);
			}
			break;
		case NFAPI_RX_ULSCH_INDICATION:
			{
				nfapi_rx_indication_t* rx_ind = (nfapi_rx_indication_t*)(header);
				size_t number_of_pdus = rx_ind->rx_indication_body.number_of_pdus;
                                assert(number_of_pdus <= NFAPI_RX_IND_MAX_PDU);
				for(size_t i = 0; i < number_of_pdus; ++i)
				{
					vnf_p7_codec_free(vnf_p7, rx_ind->rx_indication_body.rx_pdu_list[i].rx_ind_data);
				}

				vnf_p7_codec_free(vnf_p7, rx_ind->rx_indication_body.rx_pdu_list);
			}
			break;
		case NFAPI_RACH_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_rach_indication_t*)(header))->rach_indication_body.preamble_list);
			}
			break;
		case NFAPI_SRS_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_srs_indication_t*)(header))->srs_indication_body.srs_pdu_list);
			}
			break;
		case NFAPI_RX_SR_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_sr_indication_t*)(header))->sr_indication_body.sr_pdu_list);
			}
			break;
		case NFAPI_RX_CQI_INDICATION:
			{
				vnf_p7_codec_free(vnf_p7, ((nfapi_cqi_indication_t*)(header))->cqi_indication_body.cqi_pdu_list);
				vnf_p7_codec_free(vnf_p7, ((nfapi_cqi_indication_t*)(header))->cqi_indication_body.cqi_raw_pdu_list);
			}
			break;
	}

	vnf_p7_free(vnf_p7, header);

}
