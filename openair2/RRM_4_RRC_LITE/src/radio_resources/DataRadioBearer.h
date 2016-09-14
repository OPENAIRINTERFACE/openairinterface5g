#ifndef _DATARADIOBEARER_H
#    define _DATARADIOBEARER_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

#    include "Message.h"
#    include "LogicalChannel.h"
#    include "RadioBearer.h"
#    include "DRB-ToAddMod.h"

using namespace std;

class DataRadioBearer: public RadioBearer
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  DataRadioBearer ():m_id(0u) {};
  DataRadioBearer (unsigned int);
  ~DataRadioBearer ();

  friend inline bool operator>( const DataRadioBearer& l, const DataRadioBearer r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const DataRadioBearer& l, const DataRadioBearer r ) {
    return l.m_id == r.m_id;
  }


private:
  unsigned int       m_id;


  DRB_ToAddMod_t     m_drb_to_add_mod;
};
#    endif

