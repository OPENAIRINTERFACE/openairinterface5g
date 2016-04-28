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

#endif /* _UTILS_H_ */
