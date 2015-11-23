#ifndef _MESSAGE_H_
#    define _MESSAGE_H_

#    include <string>
#    include <cstring>

#    include "socket_include.h"
#    include "storage.h"
#    include "platform.h"

#define MESSAGE_DESERIALIZE_BUFFER_SIZE 2048
#define MESSAGE_SERIALIZE_BUFFER_SIZE   2048
class           Message
{
public:


  typedef std::pair<std::string,int> e_nodeb_internet_address_id_t;

  //============ STATUS ======================
  static const msg_response_status_t STATUS_CONNECTION_ACCEPTED                     = 1;
  static const msg_response_status_t STATUS_CONNECTION_FAILED                       = -1;
  static const msg_response_status_t STATUS_ADD_USER_SUCCESSFULL                    = 2;
  static const msg_response_status_t STATUS_ADD_USER_FAILED                         = -2;
  static const msg_response_status_t STATUS_REMOVE_USER_SUCCESSFULL                 = 3;
  static const msg_response_status_t STATUS_REMOVE_USER_FAILED                      = -3;
  static const msg_response_status_t STATUS_ADD_RADIO_ACCESS_BEARER_SUCCESSFULL     =  4;
  static const msg_response_status_t STATUS_ADD_RADIO_ACCESS_BEARER_FAILED          = -4;
  static const msg_response_status_t STATUS_REMOVE_RADIO_ACCESS_BEARER_SUCCESSFULL  =  5;
  static const msg_response_status_t STATUS_REMOVE_RADIO_ACCESS_BEARER_FAILED       = -5;
  //============ MESSAGES ======================
  static const msg_type_t MESSAGE_CONNECTION_REQUEST                                   = 0;
  static const msg_type_t MESSAGE_CONNECTION_RESPONSE                                  = 1;
  static const msg_type_t MESSAGE_CONNECTION_CLOSE                                     = 2;
  static const msg_type_t MESSAGE_CONNECTION_CLOSE_ACK                                 = 3;
  static const msg_type_t MESSAGE_ADD_USER_REQUEST                                     = 4;
  static const msg_type_t MESSAGE_ADD_USER_RESPONSE                                    = 5;
  static const msg_type_t MESSAGE_ADD_USER_CONFIRM                                     = 6;
  static const msg_type_t MESSAGE_REMOVE_USER_REQUEST                                  = 7;
  static const msg_type_t MESSAGE_REMOVE_USER_RESPONSE                                 = 8;
  static const msg_type_t MESSAGE_ADD_RADIO_ACCESS_BEARER_REQUEST                      = 9;
  static const msg_type_t MESSAGE_ADD_RADIO_ACCESS_BEARER_RESPONSE                     = 10;
  static const msg_type_t MESSAGE_ADD_RADIO_ACCESS_BEARER_CONFIRM                      = 11;
  static const msg_type_t MESSAGE_REMOVE_RADIO_ACCESS_BEARER_REQUEST                   = 12;
  static const msg_type_t MESSAGE_REMOVE_RADIO_ACCESS_BEARER_RESPONSE                  = 13;
  static const msg_type_t MESSAGE_MEASUREMENT_REQUEST                                  = 14;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_RG_INTERNAL_REPORT                    = 15;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_RG_QUALITY_REPORT                     = 16;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_RG_TRAFFIC_VOLUME_REPORT              = 17;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_RG_PHYSICAL_CHANNEL_DATA_AMP_REPORT   = 18;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_MT_INTERNAL_REPORT                    = 19;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_MT_QUALITY_REPORT                     = 20;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_MT_TRAFFIC_VOLUME_REPORT              = 21;
  static const msg_type_t MESSAGE_L1_MEASUREMENT_MT_INTRA_FREQUENCY_REPORT             = 22;
  static const msg_type_t MESSAGE_UPLINK_PHYSICAL_CHANNEL_CONTROL                      = 23;
  static const msg_type_t MESSAGE_CONFIG_REQUEST                                       = 24;
  static const msg_type_t MESSAGE_ACK                                                  = 25;
  static const msg_type_t MESSAGE_UPDATE_DSCH                                          = 26;
  //============ REASONS ======================
  static const msg_response_reason_t    MSG_RESP_OK                     = 0;
  static const msg_response_reason_t    MSG_RESP_PROTOCOL_ERROR         = 1;
  static const msg_response_reason_t    MSG_RESP_RESSOURCES_UNAVAILABLE = 2;
  static const msg_response_reason_t    MSG_RESP_UNKNOWN_MESSAGE        = 128;
  static const msg_response_reason_t    MSG_RESP_INTERNAL_ERROR         = 255;

  static  uint8_t s_buffer[MESSAGE_DESERIALIZE_BUFFER_SIZE];

  Message ();
  Message(tcpip::Storage&);
  virtual void    Serialize () {};
  virtual void    Forward () {};
  virtual void    Serialize (char *out_bufferP, size_t* size_dataP) {};

  static Message *DeserializeRRCMessage(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP);
  static Message *DeserializeRRMMessage(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP);
  static Message *Deserialize (const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP);
  void            ParseIpParameters(struct sockaddr *sa_fromP,socklen_t sa_lenP);
  msg_length_t    GetSerializedMessageSize();
  const char*     GetSerializedMessageBuffer();
  const int       GetSrcPort() {
    return m_port_src;
  };
  const char*     GetSrcAddress() {
    return m_ip_char_src;
  };
  virtual ~ Message ();

  static std::string StatusToString(msg_response_status_t);
  static std::string ReasonToString(msg_response_reason_t);


protected:
  msg_type_t            m_type;
  bool                  m_is_ipv6;
  struct sockaddr_in    m_sockaddr_in4_src;
  struct sockaddr_in6   m_sockaddr_in6_src;
  std::string           m_ip_str_src;
  char*                 m_ip_char_src;
  int                   m_port_src;
  tcpip::Storage        m_message_storage;
  unsigned char*        m_message_buffer;

  msg_length_t         m_msg_length;
  uint8_t              m_message_serialize_buffer[MESSAGE_SERIALIZE_BUFFER_SIZE];
  //uint8_t               m_message_serialize_buffer[MESSAGE_DESERIALIZE_BUFFER_SIZE];
};

#    endif