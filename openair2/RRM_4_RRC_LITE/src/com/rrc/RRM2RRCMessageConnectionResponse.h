#ifndef _RRM2RRCMESSAGECONNECTIONRESPONSE_H
#    define _RRM2RRCMESSAGECONNECTIONRESPONSE_H

#    include "RRM-RRC-Message.h"
#    include "Message.h"
#    include "platform.h"

class RRM2RRCMessageConnectionResponse: public Message
{

public:
  RRM2RRCMessageConnectionResponse(struct sockaddr *sa_fromP, socklen_t sa_lenP, RRM_RRC_Message_t* asn1_messageP);
  RRM2RRCMessageConnectionResponse(transaction_id_t transaction_idP, msg_response_status_t statusP, msg_response_reason_t reasonP, RRM_RRC_Message_t* asn1_messageP);
  RRM2RRCMessageConnectionResponse (msg_response_status_t statusP, msg_response_reason_t reasonP, cell_id_t cell_idP=-1);
  ~RRM2RRCMessageConnectionResponse ();

  void   Serialize ();
  void   Forward();
  const msg_response_status_t GetStatus() {
    return m_status;
  };
  const msg_response_reason_t GetReason() {
    return m_reason;
  };
  const cell_id_t             GetENodeBId() {
    return m_cell_id;
  }
  const transaction_id_t      GetTransactionId() {
    return m_transaction_id;
  };

protected:
  msg_response_status_t m_status;
  msg_response_reason_t m_reason;
  cell_id_t             m_cell_id;
  transaction_id_t      m_transaction_id;

  RRM_RRC_Message_t*    m_asn1_message;
};
#endif

