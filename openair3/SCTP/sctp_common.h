/*! \file sctp_common.h
 *  \brief eNB/MME SCTP related common procedures
 *  \author Sebastien ROUX
 *  \date 2013
 *  \version 1.0
 *  @ingroup _sctp
 */

#ifndef SCTP_COMMON_H_
#define SCTP_COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>

#if defined(ENB_MODE)
# include "UTIL/LOG/log.h"
# define SCTP_ERROR(x, args...) LOG_E(SCTP, x, ##args)
# define SCTP_WARN(x, args...)  LOG_W(SCTP, x, ##args)
# define SCTP_DEBUG(x, args...) LOG_I(SCTP, x, ##args)
#else
# define SCTP_ERROR(x, args...) do { fprintf(stderr, "[SCTP][E]"x, ##args); } while(0)
# define SCTP_DEBUG(x, args...) do { fprintf(stdout, "[SCTP][D]"x, ##args); } while(0)
# define SCTP_WARN(x, args...)  do { fprintf(stdout, "[SCTP][W]"x, ##args); } while(0)
#endif

int sctp_set_init_opt(int sd, uint16_t instreams, uint16_t outstreams,
                      uint16_t max_attempts, uint16_t init_timeout);

int sctp_get_sockinfo(int sock, uint16_t *instream, uint16_t *outstream,
                      int32_t *assoc_id);

int sctp_get_peeraddresses(int sock, struct sockaddr **remote_addr,
                           int *nb_remote_addresses);

int sctp_get_localaddresses(int sock, struct sockaddr **local_addr,
                            int *nb_local_addresses);

#endif /* SCTP_COMMON_H_ */
