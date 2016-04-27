#ifndef _DATABASE_H_
#define _DATABASE_H_

/* returns an opaque pointer - truly a 'database *', see database.c */
void *parse_database(char *filename);
void dump_database(void *database);
void list_ids(void *database);
void list_groups(void *database);
void on_off(void *d, char *item, int *a, int onoff);
char *event_name_from_id(void *database, int id);

/****************************************************************************/
/* get format of an event                                                   */
/****************************************************************************/

typedef struct {
  char **type;
  char **name;
  int count;
} database_event_format;

database_event_format get_format(void *database, int event_id);

#endif /* _DATABASE_H_ */
