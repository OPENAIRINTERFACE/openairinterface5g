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

#ifndef _LOGICALCHANNEL_H
#    define _LOGICALCHANNEL_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

using namespace std;

class LogicalChannel
{
public:
  typedef enum LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate {
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps0   = 0,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8   = 1,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps16  = 2,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps32  = 3,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps64  = 4,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps128 = 5,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps256 = 6,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity= 7,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare8  = 8,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare7  = 9,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare6  = 10,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare5  = 11,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare4  = 12,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare3  = 13,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare2  = 14,
    LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_spare1  = 15
  } e_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate;

  typedef enum LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration {
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50    = 0,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms100   = 1,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms150   = 2,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms300   = 3,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms500   = 4,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms1000  = 5,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_spare2  = 6,
    LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_spare1  = 7
  } e_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration;



  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  LogicalChannel ():m_id(0u) {};
  LogicalChannel (unsigned int);
  ~LogicalChannel ();

  friend inline bool operator>( const LogicalChannel& l, const LogicalChannel r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const LogicalChannel& l, const LogicalChannel r ) {
    return l.m_id == r.m_id;
  }


private:
  unsigned int       m_id;

};
#    endif

