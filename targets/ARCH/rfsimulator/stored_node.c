/*
  Author: Laurent THOMAS, Open Cells
  copyleft: OpenAirInterface Software Alliance and it's licence
*/

#include <common/utils/simple_executable.h>

volatile int             oai_exit = 0;

int fullread(int fd, void *_buf, int count) {
  char *buf = _buf;
  int ret = 0;
  int l;

  while (count) {
    l = read(fd, buf, count);

    if (l <= 0) return -1;

    count -= l;
    buf += l;
    ret += l;
  }

  return ret;
}

void fullwrite(int fd, void *_buf, int count) {
  char *buf = _buf;
  int l;

  while (count) {
    l = write(fd, buf, count);

    if (l <= 0) {
      if (errno==EINTR)
        continue;

      if(errno==EAGAIN) {
        continue;
      } else {
        AssertFatal(false,"Lost socket\n");
      }
    } else {
      count -= l;
      buf += l;
    }
  }
}

int server_start(short port) {
  int listen_sock;
  AssertFatal((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0, "");
  int enable = 1;
  AssertFatal(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == 0, "");
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
  AssertFatal(listen(listen_sock, 5) == 0, "");
  return accept(listen_sock,NULL,NULL);
}

int client_start(char *IP, short port) {
  int sock;
  AssertFatal((sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0, "");
  struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
    htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
  addr.sin_addr.s_addr = inet_addr(IP);
  bool connected=false;

  while(!connected) {
    //LOG_I(HW,"rfsimulator: trying to connect to %s:%d\n", IP, port);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      //LOG_I(HW,"rfsimulator: connection established\n");
      connected=true;
    }

    perror("simulated node");
    sleep(1);
  }

  return sock;
}

enum  blocking_t {
  notBlocking,
  blocking
};

void setblocking(int sock, enum blocking_t active) {
  int opts;
  AssertFatal( (opts = fcntl(sock, F_GETFL)) >= 0,"");

  if (active==blocking)
    opts = opts & ~O_NONBLOCK;
  else
    opts = opts | O_NONBLOCK;

  AssertFatal(fcntl(sock, F_SETFL, opts) >= 0, "");
}

int main(int argc, char *argv[]) {
  if(argc != 4) {
    printf("Need parameters: source file, server or destination IP, TCP port\n");
    exit(1);
  }

  int fd;
  AssertFatal((fd=open(argv[1],O_RDONLY)) != -1, "file: %s", argv[1]);
  off_t fileSize=lseek(fd, 0, SEEK_END);
  int serviceSock;

  if (strcmp(argv[2],"server")==0) {
    serviceSock=server_start(atoi(argv[3]));
  } else {
    serviceSock=client_start(argv[2],atoi(argv[3]));
  }

  samplesBlockHeader_t header;
  int bufSize=100000;
  void *buff=malloc(bufSize);
  uint64_t readTS=0;

  while (1) {
    //Rewind the file to loop on the samples
    if ( lseek(fd, 0, SEEK_CUR) >= fileSize )
      lseek(fd, 0, SEEK_SET);

    // Read one block and send it
    setblocking(serviceSock, blocking);
    AssertFatal(read(fd,&header,sizeof(header)), "");
    fullwrite(serviceSock, &header, sizeof(header));
    int dataSize=sizeof(int32_t)*header.size*header.nbAnt;
    uint64_t wroteTS=header.timestamp;

    if (dataSize>bufSize) {
      void *new_buff = realloc(buff, dataSize);

      if (new_buff == NULL) {
        free(buff);
        AssertFatal(1, "Could not reallocate");
      } else {
        buff = new_buff;
      }
    }

    AssertFatal(read(fd,buff,dataSize) == dataSize, "");
    fullwrite(serviceSock, buff, dataSize);
    // Purge incoming samples
    setblocking(serviceSock, blocking);

    while(readTS < wroteTS) {
      if ( fullread(serviceSock, &header,sizeof(header)) != sizeof(header) ||
           fullread(serviceSock, buff, sizeof(int32_t)*header.size*header.nbAnt) !=
           sizeof(int32_t)*header.size*header.nbAnt
         ) {
        printf("error: %s\n", strerror(errno));
        exit(1);
      }

      readTS=header.timestamp;
    }
  }

  return 0;
}

