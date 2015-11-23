#ifndef _RRC2RRMMESSAGEADDUSERREQUEST_H
#    define _RRC2RRMMESSAGEADDUSERREQUEST_H

#    include "RRC-RRM-Message.h"
#    include "Message.h"
#    include "platform.h"

class RRC2RRMMessageAddUserRequest: public Message
{

public:
  RRC2RRMMessageAddUserRequest(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP);
  RRC2RRMMessageAddUserRequest(std::string ip_dest_strP, int port_destP, cell_id_t cell_idP, mobile_id_t mobile_idP, transaction_id_t transaction_idP);
  ~RRC2RRMMessageAddUserRequest ();

  void   Serialize ();
  void   Forward();
  const cell_id_t        GetENodeBId() {
    return m_cell_id;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }
private:
  cell_id_t             m_cell_id;
  mobile_id_t           m_mobile_id;
  transaction_id_t      m_transaction_id;
  RRC_RRM_Message_t    *m_asn1_message;
};
#endif

