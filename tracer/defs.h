#ifndef _TRACER_DEFS_H_
#define _TRACER_DEFS_H_

/* types of plots */
#define PLOT_VS_TIME   0
#define PLOT_IQ_POINTS 1

void *make_plot(int width, int height, int bufsize, char *title, int type);
void plot_set(void *plot, float *data, int len, int pos);
void iq_plot_set(void *plot, short *data, int len, int pos);
void iq_plot_set_sized(void *_plot, short *data, int len);
void iq_plot_add_point_loop(void *_plot, short i, short q);

/* returns an opaque pointer - truly a 'database *', see t_data.c */
void *parse_database(char *filename);
void dump_database(void *database);
void list_ids(void *database);
void list_groups(void *database);
void on_off(void *d, char *item, int *a, int onoff);

#endif /* _TRACER_DEFS_H_ */
