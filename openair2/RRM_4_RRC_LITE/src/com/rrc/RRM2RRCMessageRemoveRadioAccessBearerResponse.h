#ifndef _RRM2RRCMESSAGEREMOVERADIOACCESSBEARERRESPONSE_H
#    define _RRM2RRCMESSAGEREMOVERADIOACCESSBEARERRESPONSE_H

#    include "Message.h"
#    include "platform.h"
#    include "Transaction.h"

class RRM2RRCMessageRemoveRadioAccessBearerResponse: public Message
{

public:
  RRM2RRCMessageRemoveRadioAccessBearerResponse(tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);
  RRM2RRCMessageRemoveRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t );
  RRM2RRCMessageRemoveRadioAccessBearerResponse(msg_response_status_t ,
      msg_response_reason_t ,
      cell_id_t ,
      mobile_id_t ,
      rb_id_t            ,
      transaction_id_t ,
      Transaction* );


  ~RRM2RRCMessageRemoveRadioAccessBearerResponse ();

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

