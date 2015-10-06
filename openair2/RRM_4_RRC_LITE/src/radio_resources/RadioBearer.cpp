/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#include <stdio.h>
#include "RadioBearer.h"
#include "Command.h"



//-----------------------------------------------------------------
RadioBearer::RadioBearer (rb_id_t idP,
                          T_PollRetransmit_t                 t_poll_retransmitP,
                          PollPDU_t                          poll_pduP,
                          PollByte_t                         poll_byteP,
                          long                               max_retx_thresholdP,
                          T_Reordering_t                     t_reorderingP,
                          T_StatusProhibit_t                 t_status_prohibitP,
                          long                               priorityP,
                          long                               prioritized_bit_rateP,
                          long                               bucket_size_durationP,
                          long                               logical_channel_groupP,
                          Command*                           cmdP)
//-----------------------------------------------------------------
{
    m_id = idP;
    m_rlc_mode = RLC_MODE_AM;

    m_t_poll_retransmit = t_poll_retransmitP;
    m_poll_pdu = poll_pduP;
    m_poll_byte = poll_byteP;
    m_max_retx_threshold = max_retx_thresholdP;
    m_t_reordering = t_reorderingP;
    m_t_status_prohibit = t_status_prohibitP;


    m_priority              = priorityP;
    m_prioritized_bit_rate  = prioritized_bit_rateP;
    m_bucket_size_duration  = bucket_size_durationP;
    m_logical_channel_group = logical_channel_groupP;

    tcpip::Storage storage;
    storage.writeShort(20);
    storage.writeChar(m_id);
    storage.writeChar(m_rlc_mode);

    storage.writeShort(m_t_poll_retransmit);
    storage.writeShort(m_poll_pdu);
    storage.writeShort(m_poll_byte);
    storage.writeShort(m_max_retx_threshold);
    storage.writeShort(m_t_reordering);
    storage.writeShort(m_t_status_prohibit);

    storage.writeChar(m_priority);
    storage.writeShort(m_prioritized_bit_rate);
    storage.writeShort(m_bucket_size_duration);
    storage.writeChar(m_logical_channel_group);
    cmdP->AddData(storage);
}
//-----------------------------------------------------------------
RadioBearer::RadioBearer (rb_id_t                            idP,
                          rlc_mode_t                         rlc_modeP,
                          SN_FieldLength_t                   e_sn_field_lengthP,
                          T_Reordering_t                     t_reorderingP,
                          long                               priorityP,
                          long                               prioritized_bit_rateP,
                          long                               bucket_size_durationP,
                          long                               logical_channel_groupP,
                          Command*                           cmdP)
//-----------------------------------------------------------------
{
    m_id                    = idP;
    m_rlc_mode              = rlc_modeP;

    m_sn_field_length       = e_sn_field_lengthP;
    m_t_reordering          = t_reorderingP;

    m_priority              = priorityP;
    m_prioritized_bit_rate  = prioritized_bit_rateP;
    m_bucket_size_duration  = bucket_size_durationP;
    m_logical_channel_group = logical_channel_groupP;


    tcpip::Storage storage;
    storage.writeShort(11);
    storage.writeChar(m_id);
    storage.writeChar(m_rlc_mode);
    storage.writeChar(m_sn_field_length);
    storage.writeShort(m_t_reordering);
    storage.writeChar(m_priority);
    storage.writeShort(m_prioritized_bit_rate);
    storage.writeShort(m_bucket_size_duration);
    storage.writeChar(m_logical_channel_group);
    cmdP->AddData(storage);
}
//-----------------------------------------------------------------
RadioBearer::RadioBearer (rb_id_t                            idP,
                          SN_FieldLength_t                   e_sn_field_lengthP,
                          long                               priorityP,
                          long                               prioritized_bit_rateP,
                          long                               bucket_size_durationP,
                          long                               logical_channel_groupP,
                          Command*                           cmdP)
//-----------------------------------------------------------------
{
    m_id                    = idP;
    m_rlc_mode              = RLC_MODE_UM_UNIDIRECTIONAL_UL;

    m_sn_field_length       = e_sn_field_lengthP;

    m_priority              = priorityP;
    m_prioritized_bit_rate  = prioritized_bit_rateP;
    m_bucket_size_duration  = bucket_size_durationP;
    m_logical_channel_group = logical_channel_groupP;

    tcpip::Storage storage;
    storage.writeShort(9);
    storage.writeChar(m_id);
    storage.writeChar(m_rlc_mode);
    storage.writeChar(m_sn_field_length);
    storage.writeChar(m_priority);
    storage.writeShort(m_prioritized_bit_rate);
    storage.writeShort(m_bucket_size_duration);
    storage.writeChar(m_logical_channel_group);
    cmdP->AddData(storage);
}
/*//-----------------------------------------------------------------
void RadioBearer::SetLogicalChannelParameters(logical_channel_priority_t priorityP,
                                         e_lc_config_prioritised_bit_rate_t prioritized_bit_rateP,
                                         e_lc_config_bucket_size_duration_t bucket_size_durationP,
                                         logical_channel_group_t logical_channel_groupP)
//-----------------------------------------------------------------
{
    m_priority              = priorityP;
    m_prioritized_bit_rate  = prioritized_bit_rateP;
    m_bucket_size_duration  = bucket_size_durationP;
    m_logical_channel_group = logical_channel_groupP;
}
//-----------------------------------------------------------------
void RadioBearer::SetRlcUmParameters(rlc_mode_t rlc_modeP, e_sn_field_length_t e_sn_field_lengthP, t_reordering_t t_reorderingP)
//-----------------------------------------------------------------
{
    m_rlc_mode          = rlc_modeP;
    m_t_reordering      = t_reorderingP;
    m_sn_field_length   = e_sn_field_lengthP;
}
//-----------------------------------------------------------------
void RadioBearer::SetRlcAmParameters(
                    e_t_poll_retransmit_t            t_poll_retransmitP,
                    e_poll_pdu_t                     poll_pduP,
                    e_poll_byte_t                    poll_byteP,
                    e_ul_am_rlc_max_retx_threshold_t max_retx_thresholdP,
                    e_t_reordering_t                 t_reorderingP,
                    e_t_status_prohibit_t            t_status_prohibitP)
//-----------------------------------------------------------------
{
    m_rlc_mode = RLC_MODE_AM;
    m_t_poll_retransmit = t_poll_retransmitP;
    m_poll_pdu = poll_pduP;
    m_poll_byte = poll_byteP;
    m_max_retx_threshold = max_retx_thresholdP;
    m_t_reordering = t_reorderingP;
    m_t_status_prohibit = t_status_prohibitP;
}*/
//-----------------------------------------------------------------
RadioBearer::~RadioBearer()
//-----------------------------------------------------------------
{
}

