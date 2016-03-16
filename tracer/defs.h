#ifndef _TRACER_DEFS_H_
#define _TRACER_DEFS_H_

void *make_plot(int width, int height, int bufsize, char *title);
void plot_set(void *plot, float *data, int len, int pos);
void iq_plot_set(void *plot, short *data, int len, int pos);

/* returns an opaque pointer - truly a 'database *', see t_data.c */
void *parse_database(char *filename);
void dump_database(void *database);
void list_ids(void *database);
void list_groups(void *database);
void on_off(void *d, char *item, int *a, int onoff);

#endif /* _TRACER_DEFS_H_ */
