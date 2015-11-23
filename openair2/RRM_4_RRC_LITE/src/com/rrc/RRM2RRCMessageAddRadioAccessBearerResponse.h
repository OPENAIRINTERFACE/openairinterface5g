#ifndef _RRM2RRCMESSAGEADDRADIOACCESSBEARERRESPONSE_H
#    define _RRM2RRCMESSAGEADDRADIOACCESSBEARERRESPONSE_H

#    include "Message.h"
#    include "platform.h"
#    include "Transaction.h"

class RRM2RRCMessageAddRadioAccessBearerResponse: public Message
{

public:
  RRM2RRCMessageAddRadioAccessBearerResponse(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);
  RRM2RRCMessageAddRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t );

  RRM2RRCMessageAddRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t ,
      Transaction*);

  ~RRM2RRCMessageAddRadioAccessBearerResponse ();

  void   Serialize ();
  void   Forward();

  msg_response_status_t GetStatus() {
    return m_status;
  };
  msg_response_reason_t GetReason() {
    return m_reason;
  };
  cell_id_t             GetENodeBId() {
    return m_cell_id;
  };
  mobile_id_t           GetMobileId() {
    return m_mobile_id;
  };
  rb_id_t               GetRadioBearerId() {
    return m_radio_bearer_id;
  };
  transaction_id_t      GetTransactionId() {
    return m_transaction_id;
  };

protected:
  msg_response_status_t m_status;
  msg_response_reason_t m_reason;

  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  rb_id_t            m_radio_bearer_id;
  transaction_id_t   m_transaction_id;
  Transaction*       m_transaction_result;
};
#endif

