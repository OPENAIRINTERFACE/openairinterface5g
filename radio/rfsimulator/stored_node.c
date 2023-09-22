/*
* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The OpenAirInterface Software Alliance licenses this file to You under
* the OAI Public License, Version 1.1  (the "License"); you may not use this file
* except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.openairinterface.org/?page_id=698
*
* Author and copyright: Laurent Thomas, open-cells.com
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*-------------------------------------------------------------------------------
* For more information about the OpenAirInterface (OAI) Software Alliance:
*      contact@openairinterface.org
*/


#include <common/utils/simple_executable.h>

volatile int             oai_exit = 0;

int fullread(int fd, void *_buf, int count) {
  char *buf = _buf;
  int ret = 0;
  int l;

  while (count) {
    l = read(fd, buf, count);

    if (l <= 0)
      return -1;

    count -= l;
    buf += l;
    ret += l;
  }

  return ret;
}
#define shift 4
int32_t signal_energy(int32_t *input,uint32_t length) {
  int32_t i;
  int32_t temp,temp2;
  register simde__m64 mm0, mm1, mm2, mm3;
  simde__m64 *in = (simde__m64 *)input;
  mm0 = simde_mm_setzero_si64(); // pxor(mm0,mm0);
  mm3 = simde_mm_setzero_si64(); // pxor(mm3,mm3);

  for (i=0; i<length>>1; i++) {
    mm1 = in[i];
    mm2 = mm1;
    mm1 = _m_pmaddwd(mm1,mm1);
    mm1 = _m_psradi(mm1,shift);// shift any 32 bits blocs of the word by the value shift
    mm0 = _m_paddd(mm0,mm1);// add the two 64 bits words 4 bytes by 4 bytes
    //    mm2 = _m_psrawi(mm2,shift_DC);
    mm3 = _m_paddw(mm3,mm2);// add the two 64 bits words 2 bytes by 2 bytes
  }

  mm1 = mm0;
  mm0 = _m_psrlqi(mm0,32);
  mm0 = _m_paddd(mm0,mm1);
  temp = _m_to_int(mm0);
  temp/=length;
  temp<<=shift;   // this is the average of x^2
  // now remove the DC component
  mm2 = _m_psrlqi(mm3,32);
  mm2 = _m_paddw(mm2,mm3);
  mm2 = _m_pmaddwd(mm2,mm2);
  temp2 = _m_to_int(mm2);
  temp2/=(length*length);
  //  temp2<<=(2*shift_DC);
  temp -= temp2;
  return((temp>0)?temp:1);
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
  if(argc < 4) {
    printf("Need parameters: source file, server or destination IP, TCP port (4043), 'UL|DL' if raw 2*16bits format: UL for UL IQ, DL for DL IQs \n");
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

  bool raw = false;

  if ( argc == 5 ) {
    raw=true;
  }

  samplesBlockHeader_t header;
  int bufSize=100000;
  void *buff=malloc(bufSize);
  uint64_t timestamp=0;
  const int blockSize=1920;
  // If fileSize is not multiple of blockSize*4 then discard remaining samples
  fileSize = (fileSize/(blockSize<<2))*(blockSize<<2);

  while (1) {
    //Rewind the file to loop on the samples
    if ( lseek(fd, 0, SEEK_CUR) >= fileSize )
      lseek(fd, 0, SEEK_SET);

    // Read one block and send it
    setblocking(serviceSock, blocking);

    if ( raw ) {
      header.size=blockSize;
      header.nbAnt=1;
      header.timestamp=timestamp;
      timestamp+=blockSize;
      header.option_value=0;
      header.option_flag=0;
    } else {
      AssertFatal(read(fd,&header,sizeof(header)), "");
    }

    fullwrite(serviceSock, &header, sizeof(header));
    int dataSize=sizeof(int32_t)*header.size*header.nbAnt;

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

    if (raw) // UHD shifts the 12 ADC values in MSB
      for (int i=0; i<header.size*header.nbAnt*2; i++)
        ((int16_t *)buff)[i]/=16;

    usleep(1000);
    printf("sending at ts: %lu, number of samples: %d, energy: %d\n",
           header.timestamp, header.size, signal_energy(buff, header.size));
    fullwrite(serviceSock, buff, dataSize);
    // Purge incoming samples
    setblocking(serviceSock, notBlocking);
    int ret;

    do {
      char buff[64000];
      ret=read(serviceSock, buff, 64000);

      if ( ret<0 && !( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
        printf("error: %s\n", strerror(errno));
        exit(1);
      }
    } while ( ret > 0 ) ;
  }

  return 0;
}
