#ifndef _RRC2RRMMESSAGEREMOVEUSERREQUEST_H
#    define _RRC2RRMMESSAGEREMOVEUSERREQUEST_H

#    include "Message.h"
#    include "storage.h"
#    include "platform.h"

class RRC2RRMMessageRemoveUserRequest: public Message
{

public:
  RRC2RRMMessageRemoveUserRequest(std::string ip_dest_strP, int port_destP,
                                  cell_id_t ,
                                  mobile_id_t ,
                                  transaction_id_t );

  RRC2RRMMessageRemoveUserRequest (tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);

  ~RRC2RRMMessageRemoveUserRequest ();

  void                   Serialize ();
  void                   Forward();
  const unsigned int     GetENodeBId() {
    return 1u;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }

protected:
  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  transaction_id_t   m_transaction_id;
};
#endif

