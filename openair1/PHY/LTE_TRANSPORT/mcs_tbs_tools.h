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
#define modOrder(I_MCS,I_TBS) ((I_MCS-I_TBS)*2+2) // Find modulation order from I_TBS and I_MCS

/** \fn unsigned char I_TBS2I_MCS(unsigned char I_TBS);
\brief This function maps I_tbs to I_mcs according to Table 7.1.7.1-1 in 3GPP TS 36.213 V8.6.0. Where there is two supported modulation orders for the same I_TBS then either high or low modulation is chosen by changing the equality of the two first comparisons in the if-else statement.
\param I_TBS Index of Transport Block Size
\return I_MCS given I_TBS
*/
unsigned char I_TBS2I_MCS(unsigned char I_TBS);

/** \fn unsigned char SE2I_TBS(float SE,
        unsigned char N_PRB,
        unsigned char symbPerRB);
\brief This function maps a requested throughput in number of bits to I_tbs. The throughput is calculated as a function of modulation order, RB allocation and number of symbols per RB. The mapping orginates in the "Transport block size table" (Table 7.1.7.2.1-1 in 3GPP TS 36.213 V8.6.0)
\param SE Spectral Efficiency (before casting to integer, multiply by 1024, remember to divide result by 1024!)
\param N_PRB Number of PhysicalResourceBlocks allocated \sa lte_frame_parms->N_RB_DL
\param symbPerRB Number of symbols per resource block allocated to this channel
\return I_TBS given an SE and an N_PRB
*/
unsigned char SE2I_TBS(float SE,
                       unsigned char N_PRB,
                       unsigned char symbPerRB);
