#ifndef _RRC2RRMMESSAGECONNECTIONREQUEST_H
#    define _RRC2RRMMESSAGECONNECTIONREQUEST_H

#    include "RRC-RRM-Message.h"
#    include "Message.h"
#    include "storage.h"
#    include "platform.h"

class RRC2RRMMessageConnectionRequest: public Message
{

public:
  RRC2RRMMessageConnectionRequest(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRC_RRM_Message_t* asn1_messageP);
  RRC2RRMMessageConnectionRequest (std::string ip_dest_strP, int port_destP, transaction_id_t transaction_idP);
  ~RRC2RRMMessageConnectionRequest ();

  void   Serialize ();
  void   Forward();
  const  transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }
private:
  //cell_id_t m_cell_id;
  transaction_id_t     m_transaction_id;
  RRC_RRM_Message_t    *m_asn1_message;
};
#endif

