#ifndef _RRC2RRMMESSAGEADDRADIOACCESSBEARERCONFIRM_H
#    define _RRC2RRMMESSAGEADDRADIOACCESSBEARERCONFIRM_H

#    include "Message.h"
#    include "storage.h"
#    include "platform.h"

class RRC2RRMMessageAddRadioAccessBearerConfirm: public Message
{

public:
  RRC2RRMMessageAddRadioAccessBearerConfirm(std::string ip_dest_strP, int port_destP,
      cell_id_t cell_idP,
      mobile_id_t mobile_idP,
      rb_id_t          radio_bearer_idP,
      transaction_id_t transaction_idP);
  RRC2RRMMessageAddRadioAccessBearerConfirm (tcpip::Storage& in_messageP, msg_length_t msg_lengthP, frame_t msg_frameP, struct sockaddr *sa_fromP, socklen_t sa_lenP);
  ~RRC2RRMMessageAddRadioAccessBearerConfirm ();

  void   Serialize ();
  void   Forward();
  const unsigned int     GetENodeBId() {
    return m_cell_id;
  }
  const mobile_id_t      GetMobileId() {
    return m_mobile_id;
  }
  const rb_id_t          GetRadioBearerId() {
    return m_radio_bearer_id;
  }
  const transaction_id_t GetTransactionId() {
    return m_transaction_id;
  }

private:
  cell_id_t          m_cell_id;
  mobile_id_t        m_mobile_id;
  rb_id_t            m_radio_bearer_id;
  transaction_id_t   m_transaction_id;
};
#endif

