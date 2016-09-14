#ifndef _UE_IP_CST
#define _UE_IP_CST

#define UE_IP_MAX_LENGTH 180

// General Constants
#define UE_IP_MTU                    1500
#define UE_IP_TX_QUEUE_LEN           100
#define UE_IP_ADDR_LEN               8
#define UE_IP_INET6_ADDRSTRLEN       46
#define UE_IP_INET_ADDRSTRLEN        16
#define UE_IP_DEFAULT_RAB_ID         1

#define UE_IP_RESET_RX_FLAGS         0


#define UE_IP_RETRY_LIMIT_DEFAULT    (int)5

#define UE_IP_MESSAGE_MAXLEN         (int)5004

#define UE_IP_TIMER_ESTABLISHMENT_DEFAULT (int)12
#define UE_IP_TIMER_RELEASE_DEFAULT       (int)2
#define UE_IP_TIMER_IDLE                  UINT_MAX
#define UE_IP_TIMER_TICK                  HZ

#define UE_IP_PDCPH_SIZE                  (int)sizeof(struct pdcp_data_req_header_s)
#define UE_IP_IPV4_SIZE                   (int)20
#define UE_IP_IPV6_SIZE                   (int)40




#define UE_IP_NB_INSTANCES_MAX       8


#endif

