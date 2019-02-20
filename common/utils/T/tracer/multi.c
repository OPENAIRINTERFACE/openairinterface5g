#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "database.h"
#include "utils.h"
#include "../T.h"
#include "config.h"

#define DEFAULT_LOCAL_PORT 2022

typedef struct {
  int id;         /* increases at each new tracer's connection */
  int s;          /* socket */
  int *is_on;     /* local vision of is_on for this tracer */
  int poll_id;    /* -1: invalid, otherwise index in fds array */
} ti_t;

typedef struct {
  ti_t *ti;         /* data for tracers */
  int  ti_size;
  int  ti_maxsize;
} multi_t;

void set_is_on(int *is_on, int pos, int val)
{
  if (val) is_on[pos]++; else is_on[pos]--;
  /* TODO: remove check? */
  if (is_on[pos] < 0) { printf("%s:%d:nonono\n",__FILE__,__LINE__); abort(); }
}

int send_messages_txt(int s, char *T_messages_txt, int T_messages_txt_len)
{
  char buf[T_BUFFER_MAX];
  char *T_LOCAL_buf = buf;
  int32_t T_LOCAL_size;
  unsigned char *src;
  int src_len;

  /* trace T_message.txt
   * Send several messages -1 with content followed by message -2.
   */
  src = (unsigned char *)T_messages_txt;
  src_len = T_messages_txt_len;
  while (src_len) {
    int send_size = src_len;
    if (send_size > T_PAYLOAD_MAXSIZE - sizeof(int))
      send_size = T_PAYLOAD_MAXSIZE - sizeof(int);
    /* TODO: be careful, we use internal T stuff, to rewrite? */
    T_LOCAL_size = 0;
    T_HEADER(T_ID(-1));
    T_PUT_buffer(1, ((T_buffer){addr:(src), length:(send_size)}));
    if (socket_send(s, &T_LOCAL_size, 4) == -1) return -1;
    if (socket_send(s, buf, T_LOCAL_size) == -1) return -1;
    src += send_size;
    src_len -= send_size;
  }
  T_LOCAL_size = 0;
  T_HEADER(T_ID(-2));
  if (socket_send(s, &T_LOCAL_size, 4) == -1) return -1;
  return socket_send(s, buf, T_LOCAL_size);
}

void new_tracer(multi_t *m, int s, int is_on_size, char *t, int t_size, int id)
{
  if (send_messages_txt(s, t, t_size) == -1) {
    printf("error sending T_messages.txt to new tracer %d => remove tracer\n",
           id);
    return;
  }
  if (m->ti_size == m->ti_maxsize) {
    m->ti_maxsize += 64;
    m->ti = realloc(m->ti, m->ti_maxsize * sizeof(ti_t));
    if (m->ti == NULL) abort();
  }
  m->ti[m->ti_size].id = id;
  m->ti[m->ti_size].s = s;
  m->ti[m->ti_size].is_on = calloc(is_on_size, sizeof(int));
  if (m->ti[m->ti_size].is_on == NULL) abort();
  m->ti[m->ti_size].poll_id = -1;
  m->ti_size++;
}

void remove_tracer(multi_t *m, int t)
{
  free(m->ti[t].is_on);
  shutdown(m->ti[t].s, SHUT_RDWR);
  close(m->ti[t].s);
  m->ti_size--;
  memmove(&m->ti[t], &m->ti[t+1], (m->ti_size - t) * sizeof(ti_t));
}

int send_is_on(int socket, int number_of_events, int *is_on)
{
  int i;
  char mt = 1;
  if (socket_send(socket, &mt, 1) == -1 ||
      socket_send(socket, &number_of_events, sizeof(int)) == -1) return -1;
  for (i = 0; i < number_of_events; i++) {
    int v = is_on[i] ? 1 : 0;
    if (socket_send(socket, &v, sizeof(int)) == -1) return -1;
  }
  return 0;
}

int read_tracee(int s, OBUF *ebuf, int *_type, int32_t *_length)
{
  int type;
  int32_t length;
  char *v;
  int vpos = 0;

  if (fullread(s, &length, 4) == -1) return -1;
  if (ebuf->omaxsize < length) {
    ebuf->omaxsize = (length + 65535) & ~65535;
    ebuf->obuf = realloc(ebuf->obuf, ebuf->omaxsize);
    if (ebuf->obuf == NULL) { printf("out of memory\n"); exit(1); }
  }
  v = ebuf->obuf;
  memcpy(v+vpos, &length, 4);
  vpos += 4;
#ifdef T_SEND_TIME
  if (fullread(s,v+vpos,sizeof(struct timespec))==-1) return -1;
  vpos += sizeof(struct timespec);
  length -= sizeof(struct timespec);
#endif
  if (fullread(s, &type, sizeof(int)) == -1) return -1;
  memcpy(v+vpos, &type, sizeof(int));
  vpos += sizeof(int);
  length -= sizeof(int);
  if (fullread(s, v+vpos, length) == -1) return -1;
  vpos += length;

  ebuf->osize = vpos;

  *_type = type;
  *_length = length;
  return 0;
}

void forward_event(multi_t *m, int number_of_events, OBUF *ebuf, int type)
{
  int i;

  if (type < 0 || type >= number_of_events)
    { printf("error: bad type of event to forward %d\n", type); abort(); }

  for (i = 0; i < m->ti_size; i++) {
    if (!m->ti[i].is_on[type]) continue;
    if (socket_send(m->ti[i].s, ebuf->obuf, ebuf->osize) == -1)
      printf("warning: error forwarding event to tracer %d\n", m->ti[i].id);
  }
}

int connect_to_tracee(char *ip, int port, int number_of_events, int *is_on)
{
  int s;

  printf("connecting to %s:%d\n", ip, port);

  s = try_connect_to(ip, port);
  if (s == -1) return -1;

  if (send_is_on(s, number_of_events, is_on) == -1) {
    shutdown(s, SHUT_RDWR);
    close(s);
    return -1;
  }

  return s;
}

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p <port>                 connect to given port (default %d)\n"
"    -lp <port>                listen on local port (default %d)\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT,
  DEFAULT_LOCAL_PORT
  );
  exit(1);
}

int main(int n, char **v)
{
  char *database_filename = NULL;
  void *database;
  char *ip = DEFAULT_REMOTE_IP;
  int  port = DEFAULT_REMOTE_PORT;
  int  local_port = DEFAULT_LOCAL_PORT;
  int  *is_on;
  int  number_of_events;
  int  i, j;
  char *T_messages_txt;
  int  T_messages_txt_len;
  int  l;           /* listen socket for tracers' connections */
  int  s = -1;      /* socket connected to tracee. -1 if not connected */
  multi_t m;
  int is_on_changed;
  int current_nfd;
  struct pollfd *fds = NULL;
  int next_id = 0;

  memset(&m, 0, sizeof(m));

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-lp"))
      { if (i > n-2) usage(); local_port = atoi(v[++i]); continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  load_config_file(database_filename);
  get_local_config(&T_messages_txt, &T_messages_txt_len);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  /* create listener socket */
  l = create_listen_socket("0.0.0.0", local_port);

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

  current_nfd = 0;

  while (1) {
    int nfd;
    int timeout;

    if (s == -1) s = connect_to_tracee(ip, port, number_of_events, is_on);

    /* poll on s (if there), l, and all tracers' sockets */
    nfd = 1 + (s != -1) + m.ti_size;
    if (nfd != current_nfd) {
      current_nfd = nfd;
      free(fds);
      fds = calloc(nfd, sizeof(struct pollfd));
      if (fds == NULL) { perror("calloc"); exit(1); }
    }
    i = 0;
    fds[i].fd = l;
    fds[i].events = POLLIN;
    i++;
    if (s != -1) {
      fds[i].fd = s;
      fds[i].events = POLLIN;
      i++;
    }
    for (j = 0; j < m.ti_size; j++) {
      m.ti[j].poll_id = i;
      fds[i].fd = m.ti[j].s;
      fds[i].events = POLLIN;
      i++;
    }
    if (s == -1) timeout = 1000; else timeout = -1;
    if (poll(fds, nfd, timeout) == -1) { perror("poll"); exit(1); }

    if (fds[0].revents & ~POLLIN) {
      printf("TODO: error on listen socket?\n");
      exit(1);
    }

    /* new tracer connecting? */
    if (fds[0].revents & POLLIN) {
      int t;
      printf("tracer %d connecting\n", next_id);
      t = socket_accept(l);
      if (t == -1) perror("accept");
      else new_tracer(&m, t, number_of_events,
                      T_messages_txt, T_messages_txt_len, next_id);
      next_id++;
    }

    if (s != -1 && fds[1].revents & ~POLLIN) {
      printf("TODO: error on tracee socket?\n");
      exit(1);
    }

    /* data from tracee */
    if (s != -1 && fds[1].revents & POLLIN) {
      int type;
      int32_t length;
      if (read_tracee(s, &ebuf, &type, &length) == -1) {
        clear_remote_config();
        shutdown(s, SHUT_RDWR);
        close(s);
        s = -1;
      } else {
        if (type == -1)
          append_received_config_chunk(ebuf.obuf+ebuf.osize-length, length);
        else if (type == -2) verify_config();
        else forward_event(&m, number_of_events, &ebuf, type);
      }
    }

    /* status of each tracer */
    is_on_changed = 0;
    for (j = 0; j < m.ti_size; j++) {
      int l;
      int s;
      int *t_is_on;
      if (m.ti[j].poll_id == -1) continue;
      i = m.ti[j].poll_id;
      s = m.ti[j].s;
      t_is_on = m.ti[j].is_on;
      if (fds[i].revents & (POLLHUP | POLLERR)) goto tracer_error;
      /* error? */
      if (fds[i].revents & ~POLLIN) {
        printf("TODO: error with tracer?\n");
        exit(1);
      }
      /* data in */
      if (fds[i].revents & POLLIN) {
        char t;
        int len;
        int v;
        if (fullread(s, &t, 1) != 1) goto tracer_error;
        switch (t) {
        case 0:
          is_on_changed = 1;
          if (fullread(s, &len, sizeof(int)) == -1) goto tracer_error;
          for (l = 0; l < len; l++) {
            if (fullread(s, &v, sizeof(int)) == -1) goto tracer_error;
            if (v < 0 || v >= number_of_events) goto tracer_error;
            t_is_on[v] = 1 - t_is_on[v];
            set_is_on(is_on, v, t_is_on[v]);
          }
          break;
        case 1:
          is_on_changed = 1;
          if (fullread(s, &len, sizeof(int)) == -1) goto tracer_error;
          if (len < 0 || len > number_of_events) goto tracer_error;
          for (l = 0; l < len; l++) {
            if (fullread(s, &v, sizeof(int)) == -1) goto tracer_error;
            if (v < 0 || v > 1) goto tracer_error;
            if (t_is_on[l] != v) set_is_on(is_on, l, v);
            t_is_on[l] = v;
          }
          break;
        case 2: break;
        default: printf("error: unhandled message type %d\n", t); //abort();
        }
      }
      continue;
tracer_error:
      printf("remove tracer %d\n", m.ti[j].id);
      for (l = 0; l < number_of_events; l++)
        if (m.ti[j].is_on[l]) { is_on_changed = 1; set_is_on(is_on, l, 0); }
      remove_tracer(&m, j);
      j--;
    }
    if (is_on_changed && s != -1)
      if (send_is_on(s, number_of_events, is_on) == -1) {
        clear_remote_config();
        shutdown(s, SHUT_RDWR);
        close(s);
        s = -1;
      }
  }

  return 0;
}
