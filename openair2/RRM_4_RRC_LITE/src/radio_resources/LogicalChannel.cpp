#include <stdio.h>
#include "LogicalChannel.h"


//-----------------------------------------------------------------
LogicalChannel::LogicalChannel(unsigned int idP)
//-----------------------------------------------------------------
{
    m_id = idP;

}
//-----------------------------------------------------------------
LogicalChannel::~LogicalChannel()
//-----------------------------------------------------------------
{
    //if (!m_radio_bearer_map.empty())  m_radio_bearer_map.clear();
}

