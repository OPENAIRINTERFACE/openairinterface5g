#ifndef _UTILS_H_
#define _UTILS_H_

void new_thread(void *(*f)(void *), void *data);
void sleepms(int ms);

/****************************************************************************/
/* list                                                                     */
/****************************************************************************/

typedef struct list {
  struct list *last, *next;
  void *data;
} list;

list *list_remove_head(list *l);
list *list_append(list *l, void *data);

/****************************************************************************/
/* socket                                                                   */
/****************************************************************************/

void socket_send(int socket, void *buffer, int size);

/****************************************************************************/
/* buffer                                                                   */
/****************************************************************************/

typedef struct {
  int osize;
  int omaxsize;
  char *obuf;
} OBUF;

void PUTC(OBUF *o, char c);
void PUTS(OBUF *o, char *s);
void PUTS_CLEAN(OBUF *o, char *s);
void PUTI(OBUF *o, int i);

#endif /* _UTILS_H_ */
