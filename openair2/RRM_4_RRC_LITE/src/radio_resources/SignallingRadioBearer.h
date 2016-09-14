#ifndef _SIGNALLINGRADIOBEARER_H
#    define _SIGNALLINGRADIOBEARER_H

#    include <boost/ptr_container/ptr_map.hpp>
#    include <map>
//#        include <boost/shared_ptr.hpp>

#    include "Message.h"
#    include "LogicalChannel.h"
#    include "RadioBearer.h"
#    include "SRB-ToAddMod.h"

using namespace std;

class SignallingRadioBearer: public RadioBearer
{
public:
  //typedef boost::shared_ptr<ENodeB> ENodeBPtr;

  SignallingRadioBearer ():m_id(0u) {};
  SignallingRadioBearer (unsigned int);
  ~SignallingRadioBearer ();

  friend inline bool operator>( const SignallingRadioBearer& l, const SignallingRadioBearer r ) {
    return l.m_id > r.m_id;
  }
  friend inline bool operator==( const SignallingRadioBearer& l, const SignallingRadioBearer r ) {
    return l.m_id == r.m_id;
  }


private:
  unsigned int       m_id;


  SRB_ToAddMod_t     m_srb_to_add_mod;
};
#    endif

