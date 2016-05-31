#ifndef _FORWARD_H_
#define _FORWARD_H_

void *forwarder(int port);
void forward(void *forwarder, char *buf, int size);
void forward_start_client(void *forwarder, int socket);

#endif /* _FORWARD_H_ */
