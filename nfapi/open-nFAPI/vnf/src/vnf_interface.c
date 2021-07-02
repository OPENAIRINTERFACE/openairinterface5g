/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <stdio.h>

#include "vnf.h"



nfapi_vnf_config_t* nfapi_vnf_config_create()
{
	vnf_t* _this = (vnf_t*)calloc(1, sizeof(vnf_t));

	if(_this == 0)
		return 0;

	_this->sctp = 1;

	_this->next_phy_id = 1;
	
	// Set the default P5 port
	_this->_public.vnf_p5_port = NFAPI_P5_SCTP_PORT;
	
	// set the default memory allocation 
	_this->_public.malloc = &malloc;
	_this->_public.free = &free;
	
	// set the default memory allocation 
	_this->_public.codec_config.allocate = &malloc;
	_this->_public.codec_config.deallocate = &free;
	

	return (nfapi_vnf_config_t* )_this;
}

void nfapi_vnf_config_destory(nfapi_vnf_config_t* config)
{
	free(config);
}

int nfapi_nr_vnf_start(nfapi_vnf_config_t* config)
{
	// Verify that config is not null
	if(config == 0)
		return -1;

	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

	int p5ListenSock, p5Sock; 

	struct sockaddr_in addr;
	socklen_t addrSize;

	struct sockaddr_in6 addr6;

	struct sctp_event_subscribe events;
	struct sctp_initmsg initMsg;
	int noDelay;

	(void)memset(&addr, 0, sizeof(struct sockaddr_in));
	(void)memset(&addr6, 0, sizeof(struct sockaddr_in6));
	(void)memset(&events, 0, sizeof(struct sctp_event_subscribe));
	(void)memset(&initMsg, 0, sizeof(struct sctp_initmsg));

	vnf_t* vnf = (vnf_t*)(config);

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Starting P5 VNF connection on port %u\n", config->vnf_p5_port);

	/*
	char * host = 0;
	char * port = "4242";
	struct addrinfo hints;
	bzero(&hints, sizeof(struct addrinfo));
	//hints.ai_flags=AI_PASSIVE;
	//hints.ai_flags=AI_DEFAULT;
	hints.ai_family=AF_UNSPEC;
	//hints.ai_family=AF_INET6;
	hints.ai_socktype=SOCK_STREAM;
	//hints.ai_protocol=IPPROTO_SCTP

	struct addrinfo *aiHead = 0;



	int result = getaddrinfo(host, port, &hints, &aiHead);
	NFAPI_TRACE(NFAPI_TRACE_INFO, "getaddrinfo return %d %d\n", result, errno);

	while(aiHead->ai_next != NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "addr info %d (IP %d UDP %d SCTP %d)\n %d (%d)\n", 
				aiHead->ai_protocol, IPPROTO_IP, IPPROTO_UDP, IPPROTO_SCTP, 
				aiHead->ai_flags, AI_PASSIVE);

		char hostBfr[ NI_MAXHOST ];
		char servBfr[ NI_MAXSERV ];

		getnameinfo(aiHead->ai_addr,
				aiHead->ai_addrlen,
				hostBfr,
				sizeof( hostBfr ),
				servBfr,
				sizeof( servBfr ),
				NI_NUMERICHOST | NI_NUMERICSERV );

		switch(aiHead->ai_family)
		{
			case PF_INET:
				{
				struct sockaddr_in *pSadrIn = (struct sockaddr_in*) aiHead->ai_addr;
				printf(
						"   ai_addr      = sin_family: %d (AF_INET = %d, "
						"AF_INET6 = %d)\n"
						"                  sin_addr:   %s\n"
						"                  sin_port:   %s\n",
						pSadrIn->sin_family,
						AF_INET,
						AF_INET6,
						hostBfr,
						servBfr );
				}
				break;
			case PF_INET6:
				{
				struct sockaddr_in6 *pSadrIn6 = (struct sockaddr_in6*) aiHead->ai_addr;
				fprintf( stderr,
						"   ai_addr      = sin6_family:   %d (AF_INET = %d, "
						"AF_INET6 = %d) \n"
						"                  sin6_addr:     %s\n"
						"                  sin6_port:     %s\n"
						"                  sin6_flowinfo: %d\n"
						"                  sin6_scope_id: %d\n",
						pSadrIn6->sin6_family,
						AF_INET,
						AF_INET6,
						hostBfr,
						servBfr,
						pSadrIn6->sin6_flowinfo,
						pSadrIn6->sin6_scope_id);
				}
				break;
			default:
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Not ment to be here\n");
				break;
		}

		aiHead = aiHead->ai_next;
	}
	*/

	{
		int protocol;
		int domain;

		if (vnf->sctp)
			protocol = IPPROTO_SCTP;
		else
			protocol = IPPROTO_IP;

		if(config->vnf_ipv6)
		{
			domain = PF_INET6;
		}
		else
		{
			domain = AF_INET;
		}

		// open the SCTP socket
		if ((p5ListenSock = socket(domain, SOCK_STREAM, protocol)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P5 socket errno: %d\n", errno);
			return 0;
		}
		NFAPI_TRACE(NFAPI_TRACE_INFO, "P5 socket created... %d\n", p5ListenSock);
	}

	if (vnf->sctp)
	{
		// configure for MSG_NOTIFICATION
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_EVENTS) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		NFAPI_TRACE(NFAPI_TRACE_NOTE, "VNF Setting the SCTP_INITMSG\n");
		// configure the SCTP socket options
		initMsg.sinit_num_ostreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
		initMsg.sinit_max_instreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_INITMSG) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		noDelay = 1;
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (STCP_NODELAY) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		struct sctp_event_subscribe events;
		memset( (void *)&events, 0, sizeof(events) );
  	    events.sctp_data_io_event = 1;
		
		if(setsockopt(p5ListenSock, SOL_SCTP, SCTP_EVENTS, (const void *)&events, sizeof(events)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
			close(p5ListenSock);
			return -1;
		}

	}


	if(config->vnf_ipv6)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV6 binding to port %d %d\n", config->vnf_p5_port, p5ListenSock);
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(config->vnf_p5_port);
		addr6.sin6_addr = in6addr_any;

		// bind to the configured address and port
		if (bind(p5ListenSock, (struct sockaddr *)&addr6, sizeof(struct sockaddr_in6)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
	}
	else if(config->vnf_ipv4)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV4 binding to port %d\n", config->vnf_p5_port);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(config->vnf_p5_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		// bind to the configured address and port
		if (bind(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
		//if (sctp_bindx(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), SCTP_BINDX_ADD_ADDR) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "bind succeeded..%d.\n", p5ListenSock);

	// put the socket into listen mode
	if (listen(p5ListenSock, 2) < 0) 
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After listen errno: %d\n", errno);
		close(p5ListenSock);
		return 0;
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "listen succeeded...\n");

	struct timeval tv;
	fd_set read_fd_set;


	int p5_idx = 0;
	while(vnf->terminate == 0)
	{
		FD_ZERO(&read_fd_set);

		FD_SET(p5ListenSock, &read_fd_set);
		int max_fd = p5ListenSock;

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
		while(pnf != 0)
		{
			if(pnf->connected)
			{
				FD_SET(pnf->p5_sock, &read_fd_set);
				if (pnf->p5_sock > max_fd)
				{
					max_fd = pnf->p5_sock;
				}
			}

			pnf = pnf->next;
		}

		int select_result = select(max_fd + 1, &read_fd_set, 0, 0, &tv);

		if(select_result == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "select result %d errno %d\n", select_result, errno);
			close(p5ListenSock);
			return 0;
		}
		else if(select_result)
		{
			if(FD_ISSET(p5ListenSock, &read_fd_set))
			{
				addrSize = sizeof(struct sockaddr_in);
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Accepting connection from PNF...\n");

				p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);

				if (p5Sock < 0) 
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to accept PNF connection reason:%d\n", errno);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF connection (fd:%d) accepted from %s:%d \n", p5Sock,  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
					nfapi_vnf_pnf_info_t* pnf = (nfapi_vnf_pnf_info_t*)malloc(sizeof(nfapi_vnf_pnf_info_t));
					NFAPI_TRACE(NFAPI_TRACE_INFO, "MALLOC nfapi_vnf_pnf_info_t for pnf_list pnf:%p\n", pnf);
					memset(pnf, 0, sizeof(nfapi_vnf_pnf_info_t));
					pnf->p5_sock = p5Sock;
					pnf->p5_idx = p5_idx++;
					pnf->p5_pnf_sockaddr = addr;
					pnf->connected = 1;

					nfapi_vnf_pnf_list_add(config, pnf);

					// Inform mac that a pnf connection has been established
					// todo : allow mac to 'accept' the connection. i.e. to
					// reject it.
					if(config->pnf_nr_connection_indication != 0)
					{
						(config->pnf_nr_connection_indication)(config, pnf->p5_idx);
					}

					
					// check the connection status
					{
						struct sctp_status status;
						(void)memset(&status, 0, sizeof(struct sctp_status));
						socklen_t optLen = (socklen_t) sizeof(struct sctp_status);
						if (getsockopt(p5Sock, IPPROTO_SCTP, SCTP_STATUS, &status, &optLen) < 0)
						{
							NFAPI_TRACE(NFAPI_TRACE_ERROR, "After getsockopt errno: %d\n", errno);
							return -1;
						}
						else
						{
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Association ID = %d\n", status.sstat_assoc_id);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Receiver window size = %d\n", status.sstat_rwnd);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF In Streams = %d\n",  status.sstat_instrms);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Out Streams = %d\n", status.sstat_outstrms);

						}
					}
				}
			}
			else
			{
				uint8_t delete_pnfs = 0;

				nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
				while(pnf != 0)
				{
					if(FD_ISSET(pnf->p5_sock, &read_fd_set))
					{
						if(vnf_nr_read_dispatch_message(config, pnf) == 0)
						{
							if(config->pnf_disconnect_indication != 0)
							{
								(config->pnf_disconnect_indication)(config, pnf->p5_idx);
							}

							close(pnf->p5_sock);

							pnf->to_delete = 1;
							delete_pnfs = 1;
						}
					}
			
					pnf = pnf->next;
				}

				if(delete_pnfs)
				{
					nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
					nfapi_vnf_pnf_info_t* prev = 0;
					while(pnf != 0)
					{
						nfapi_vnf_pnf_info_t* curr = pnf;

						if(pnf->to_delete == 1)
						{
							if(prev == 0)
							{
								config->pnf_list = pnf->next;
							}
							else
							{
								prev->next = pnf->next;
							}

							pnf = pnf->next;

							free(curr);
						}
						else
						{
							prev = pnf;
							pnf = pnf->next;
						}

					}
					
				}
			}

			continue;
		}
		else
		{
			// timeout
			
			// Should we test for socket closure here every second?

			continue;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Sock socket's\n");
	{
		nfapi_vnf_pnf_info_t* curr = config->pnf_list;
		while(curr != NULL)
		{
			if(config->pnf_disconnect_indication)
			{
				(config->pnf_disconnect_indication)(config, curr->p5_idx);
			}

			close(curr->p5_sock);
			curr = curr->next;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Listen socket\n");
	close(p5ListenSock);
		
	return 0;

}

int nfapi_vnf_start(nfapi_vnf_config_t* config)
{
	assert(config != 0);
	NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

	int p5ListenSock, p5Sock; 

	struct sockaddr_in addr;
	socklen_t addrSize;

	struct sockaddr_in6 addr6;

	struct sctp_event_subscribe events;
	struct sctp_initmsg initMsg;
	int noDelay;

	(void)memset(&addr, 0, sizeof(struct sockaddr_in));
	(void)memset(&addr6, 0, sizeof(struct sockaddr_in6));
	(void)memset(&events, 0, sizeof(struct sctp_event_subscribe));
	(void)memset(&initMsg, 0, sizeof(struct sctp_initmsg));

	vnf_t* vnf = (vnf_t*)(config);

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Starting P5 VNF connection on port %u\n", config->vnf_p5_port);

	/*
	char * host = 0;
	char * port = "4242";
	struct addrinfo hints;
	bzero(&hints, sizeof(struct addrinfo));
	//hints.ai_flags=AI_PASSIVE;
	//hints.ai_flags=AI_DEFAULT;
	hints.ai_family=AF_UNSPEC;
	//hints.ai_family=AF_INET6;
	hints.ai_socktype=SOCK_STREAM;
	//hints.ai_protocol=IPPROTO_SCTP

	struct addrinfo *aiHead = 0;



	int result = getaddrinfo(host, port, &hints, &aiHead);
	NFAPI_TRACE(NFAPI_TRACE_INFO, "getaddrinfo return %d %d\n", result, errno);

	while(aiHead->ai_next != NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "addr info %d (IP %d UDP %d SCTP %d)\n %d (%d)\n", 
				aiHead->ai_protocol, IPPROTO_IP, IPPROTO_UDP, IPPROTO_SCTP, 
				aiHead->ai_flags, AI_PASSIVE);

		char hostBfr[ NI_MAXHOST ];
		char servBfr[ NI_MAXSERV ];

		getnameinfo(aiHead->ai_addr,
				aiHead->ai_addrlen,
				hostBfr,
				sizeof( hostBfr ),
				servBfr,
				sizeof( servBfr ),
				NI_NUMERICHOST | NI_NUMERICSERV );

		switch(aiHead->ai_family)
		{
			case PF_INET:
				{
				struct sockaddr_in *pSadrIn = (struct sockaddr_in*) aiHead->ai_addr;
				printf(
						"   ai_addr      = sin_family: %d (AF_INET = %d, "
						"AF_INET6 = %d)\n"
						"                  sin_addr:   %s\n"
						"                  sin_port:   %s\n",
						pSadrIn->sin_family,
						AF_INET,
						AF_INET6,
						hostBfr,
						servBfr );
				}
				break;
			case PF_INET6:
				{
				struct sockaddr_in6 *pSadrIn6 = (struct sockaddr_in6*) aiHead->ai_addr;
				fprintf( stderr,
						"   ai_addr      = sin6_family:   %d (AF_INET = %d, "
						"AF_INET6 = %d) \n"
						"                  sin6_addr:     %s\n"
						"                  sin6_port:     %s\n"
						"                  sin6_flowinfo: %d\n"
						"                  sin6_scope_id: %d\n",
						pSadrIn6->sin6_family,
						AF_INET,
						AF_INET6,
						hostBfr,
						servBfr,
						pSadrIn6->sin6_flowinfo,
						pSadrIn6->sin6_scope_id);
				}
				break;
			default:
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Not ment to be here\n");
				break;
		}

		aiHead = aiHead->ai_next;
	}
	*/

	{
		int protocol;
		int domain;

		if (vnf->sctp)
			protocol = IPPROTO_SCTP;
		else
			protocol = IPPROTO_IP;

		if(config->vnf_ipv6)
		{
			domain = PF_INET6;
		}
		else
		{
			domain = AF_INET;
		}

		// open the SCTP socket
		if ((p5ListenSock = socket(domain, SOCK_STREAM, protocol)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P5 socket errno: %d\n", errno);
			return 0;
		}
		NFAPI_TRACE(NFAPI_TRACE_INFO, "P5 socket created... %d\n", p5ListenSock);
	}

	if (vnf->sctp)
	{
		// configure for MSG_NOTIFICATION
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_EVENTS) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		NFAPI_TRACE(NFAPI_TRACE_NOTE, "VNF Setting the SCTP_INITMSG\n");
		// configure the SCTP socket options
		initMsg.sinit_num_ostreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
		initMsg.sinit_max_instreams = 5; //MAX_SCTP_STREAMS;  // number of output streams can be greater
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_INITMSG) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		noDelay = 1;
		if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (STCP_NODELAY) errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
		struct sctp_event_subscribe events;
		memset( (void *)&events, 0, sizeof(events) );
  	    events.sctp_data_io_event = 1;
		
		if(setsockopt(p5ListenSock, SOL_SCTP, SCTP_EVENTS, (const void *)&events, sizeof(events)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
			close(p5ListenSock);
			return -1;
		}

	}


	if(config->vnf_ipv6)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV6 binding to port %d %d\n", config->vnf_p5_port, p5ListenSock);
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(config->vnf_p5_port);
		addr6.sin6_addr = in6addr_any;

		// bind to the configured address and port
		if (bind(p5ListenSock, (struct sockaddr *)&addr6, sizeof(struct sockaddr_in6)) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
	}
	else if(config->vnf_ipv4)
	{
		NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV4 binding to port %d\n", config->vnf_p5_port);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(config->vnf_p5_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		// bind to the configured address and port
		if (bind(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
		//if (sctp_bindx(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), SCTP_BINDX_ADD_ADDR) < 0)
		{
			NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
			close(p5ListenSock);
			return 0;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "bind succeeded..%d.\n", p5ListenSock);

	// put the socket into listen mode
	if (listen(p5ListenSock, 2) < 0) 
	{
		NFAPI_TRACE(NFAPI_TRACE_ERROR, "After listen errno: %d\n", errno);
		close(p5ListenSock);
		return 0;
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "listen succeeded...\n");

	struct timeval tv;
	fd_set read_fd_set;


	int p5_idx = 0;
	while(vnf->terminate == 0)
	{
		FD_ZERO(&read_fd_set);

		FD_SET(p5ListenSock, &read_fd_set);
		int max_fd = p5ListenSock;

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
		while(pnf != 0)
		{
			if(pnf->connected)
			{
				FD_SET(pnf->p5_sock, &read_fd_set);
				if (pnf->p5_sock > max_fd)
				{
					max_fd = pnf->p5_sock;
				}
			}

			pnf = pnf->next;
		}

		int select_result = select(max_fd + 1, &read_fd_set, 0, 0, &tv);

		if(select_result == -1)
		{
			NFAPI_TRACE(NFAPI_TRACE_INFO, "select result %d errno %d\n", select_result, errno);
			close(p5ListenSock);
			return 0;
		}
		else if(select_result)
		{
			if(FD_ISSET(p5ListenSock, &read_fd_set))
			{
				addrSize = sizeof(struct sockaddr_in);
				NFAPI_TRACE(NFAPI_TRACE_INFO, "Accepting connection from PNF...\n");

				p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);

				if (p5Sock < 0) 
				{
					NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to accept PNF connection reason:%d\n", errno);
				}
				else
				{
					NFAPI_TRACE(NFAPI_TRACE_INFO, "PNF connection (fd:%d) accepted from %s:%d \n", p5Sock,  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
					nfapi_vnf_pnf_info_t* pnf = (nfapi_vnf_pnf_info_t*)malloc(sizeof(nfapi_vnf_pnf_info_t));
					NFAPI_TRACE(NFAPI_TRACE_INFO, "MALLOC nfapi_vnf_pnf_info_t for pnf_list pnf:%p\n", pnf);
					memset(pnf, 0, sizeof(nfapi_vnf_pnf_info_t));
					pnf->p5_sock = p5Sock;
					pnf->p5_idx = p5_idx++;
					pnf->p5_pnf_sockaddr = addr;
					pnf->connected = 1;

					nfapi_vnf_pnf_list_add(config, pnf);

					// Inform mac that a pnf connection has been established
					// todo : allow mac to 'accept' the connection. i.e. to
					// reject it.
					if(config->pnf_connection_indication != 0)
					{
						(config->pnf_connection_indication)(config, pnf->p5_idx);
					}

					
					// check the connection status
					{
						struct sctp_status status;
						(void)memset(&status, 0, sizeof(struct sctp_status));
						socklen_t optLen = (socklen_t) sizeof(struct sctp_status);
						if (getsockopt(p5Sock, IPPROTO_SCTP, SCTP_STATUS, &status, &optLen) < 0)
						{
							NFAPI_TRACE(NFAPI_TRACE_ERROR, "After getsockopt errno: %d\n", errno);
							return -1;
						}
						else
						{
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Association ID = %d\n", status.sstat_assoc_id);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Receiver window size = %d\n", status.sstat_rwnd);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF In Streams = %d\n",  status.sstat_instrms);
							NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Out Streams = %d\n", status.sstat_outstrms);

						}
					}
				}
			}
			else
			{
				uint8_t delete_pnfs = 0;

				nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
				while(pnf != 0)
				{
					if(FD_ISSET(pnf->p5_sock, &read_fd_set))
					{
						if(vnf_read_dispatch_message(config, pnf) == 0)
						{
							if(config->pnf_disconnect_indication != 0)
							{
								(config->pnf_disconnect_indication)(config, pnf->p5_idx);
							}

							close(pnf->p5_sock);

							pnf->to_delete = 1;
							delete_pnfs = 1;
						}
					}
			
					pnf = pnf->next;
				}

				if(delete_pnfs)
				{
					nfapi_vnf_pnf_info_t* pnf = config->pnf_list;
					nfapi_vnf_pnf_info_t* prev = 0;
					while(pnf != 0)
					{
						nfapi_vnf_pnf_info_t* curr = pnf;

						if(pnf->to_delete == 1)
						{
							if(prev == 0)
							{
								config->pnf_list = pnf->next;
							}
							else
							{
								prev->next = pnf->next;
							}

							pnf = pnf->next;

							free(curr);
						}
						else
						{
							prev = pnf;
							pnf = pnf->next;
						}

					}
					
				}
			}

			continue;
		}
		else
		{
			// timeout
			
			// Should we test for socket closure here every second?

			continue;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Sock socket's\n");
	{
		nfapi_vnf_pnf_info_t* curr = config->pnf_list;
		while(curr != NULL)
		{
			if(config->pnf_disconnect_indication)
			{
				(config->pnf_disconnect_indication)(config, curr->p5_idx);
			}

			close(curr->p5_sock);
			curr = curr->next;
		}
	}

	NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Listen socket\n");
	close(p5ListenSock);
		
	return 0;

}

int nfapi_vnf_stop(nfapi_vnf_config_t* config)
{
	// Verify that config is not null
	if(config == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);
	_this->terminate = 1;
	return 0;
}

int nfapi_nr_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_param_request_t));
}

int nfapi_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_param_request_t));
}

int nfapi_nr_vnf_pnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_config_request_t));
}


int nfapi_vnf_pnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_config_request_t));
}

int nfapi_vnf_pnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_start_request_t));
}

int nfapi_nr_vnf_pnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_start_request_t));
}

int nfapi_vnf_pnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_stop_request_t));
}

int nfapi_nr_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_param_request_scf_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_param_request_scf_t));
}

int nfapi_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_param_request_t));
}

int nfapi_nr_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_config_request_scf_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	nfapi_vnf_phy_info_t* phy = nfapi_vnf_phy_info_list_find(config, req->header.phy_id);

	if(phy == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_WARN, "%s failed to find phy information phy_id:%d\n", __FUNCTION__, req->header.phy_id);
		return -1;
	}

	// set the timing parameters
	req->nfapi_config.timing_window.tl.tag = NFAPI_NR_NFAPI_TIMING_WINDOW_TAG;
	req->nfapi_config.timing_window.value = phy->timing_window;
	req->num_tlv++;

	req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG;
	req->nfapi_config.timing_info_mode.value = phy->timing_info_mode;
	req->num_tlv++;

	req->nfapi_config.timing_info_period.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG;
	req->nfapi_config.timing_info_period.value = phy->timing_info_period;
	req->num_tlv++;

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_config_request_scf_t));
}

int nfapi_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	nfapi_vnf_phy_info_t* phy = nfapi_vnf_phy_info_list_find(config, req->header.phy_id);

	if(phy == NULL)
	{
		NFAPI_TRACE(NFAPI_TRACE_WARN, "%s failed to find phy inforation phy_id:%d\n", __FUNCTION__, req->header.phy_id);
		return -1;
	}

	// set the timing parameters
	req->nfapi_config.timing_window.tl.tag = NFAPI_NFAPI_TIMING_WINDOW_TAG;
	req->nfapi_config.timing_window.value = phy->timing_window;
	req->num_tlv++;

	req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NFAPI_TIMING_INFO_MODE_TAG;
	req->nfapi_config.timing_info_mode.value = phy->timing_info_mode;
	req->num_tlv++;

	req->nfapi_config.timing_info_period.tl.tag = NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG;
	req->nfapi_config.timing_info_period.value = phy->timing_info_period;
	req->num_tlv++;

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_config_request_t));
}

int nfapi_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_request_t * req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_start_request_t));
}

int nfapi_nr_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_start_request_scf_t * req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_nr_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_nr_start_request_scf_t));
}


int nfapi_vnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_stop_request_t));
}
int nfapi_vnf_measurement_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_measurement_request_t));
}
int nfapi_vnf_rssi_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_rssi_request_t));
}
int nfapi_vnf_cell_search_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_cell_search_request_t));
}
int nfapi_vnf_broadcast_detect_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_broadcast_detect_request_t));
}
int nfapi_vnf_system_information_schedule_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_system_information_schedule_request_t));
}
int nfapi_vnf_system_information_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_system_information_request_t));
}
int nfapi_vnf_nmm_stop_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_request_t* req)
{
	if(config == 0 || req == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_nmm_stop_request_t));
}
int nfapi_vnf_vendor_extension(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg)
{
	if(config == 0 || msg == 0)
		return -1;

	vnf_t* _this = (vnf_t*)(config);

	return vnf_pack_and_send_p5_message(_this, p5_idx, msg, sizeof(nfapi_p4_p5_message_header_t));
}

int nfapi_vnf_allocate_phy(nfapi_vnf_config_t* config, int p5_idx, uint16_t* phy_id)
{
	vnf_t* vnf = (vnf_t*)config;

	nfapi_vnf_phy_info_t* info = (nfapi_vnf_phy_info_t*)calloc(1, sizeof(nfapi_vnf_phy_info_t));
	info->p5_idx = p5_idx;
	info->phy_id = vnf->next_phy_id++;

	info->timing_window = 30;       // This seems to override what gets set by the user - why??? //TODO: Change in NR in terms of microsecends,what should be the value?
	info->timing_info_mode = 0x03;
	info->timing_info_period = 128; // Melissa come back to this

	nfapi_vnf_phy_info_list_add(config, info);

	(*phy_id) = info->phy_id;

	return 0;
}
