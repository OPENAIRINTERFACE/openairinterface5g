#ifndef _COMMON_UTILS_T_DEFS_H_
#define _COMMON_UTILS_T_DEFS_H_

/* types of plots */
#define PLOT_VS_TIME   0
#define PLOT_IQ_POINTS 1
#define PLOT_MINMAX    2

void new_thread(void *(*f)(void *), void *data);

/* ... is { int count; int type; char *color; } for 'nplots' plots */
void *make_plot(int width, int height, char *title, int nplots, ...);
void plot_set(void *plot, float *data, int len, int pos, int pp);
void iq_plot_set(void *plot, short *data, int len, int pos, int pp);
void iq_plot_set_sized(void *_plot, short *data, int len, int pp);
void iq_plot_add_iq_point_loop(void *_plot, short i, short q, int pp);
void iq_plot_add_energy_point_loop(void *_plot, int e, int pp);

/* returns an opaque pointer - truly a 'database *', see t_data.c */
void *parse_database(char *filename);
void dump_database(void *database);
void list_ids(void *database);
void list_groups(void *database);
void on_off(void *d, char *item, int *a, int onoff);

void *forwarder(char *ip, int port);
void forward(void *forwarder, char *buf, int size);
void forward_start_client(void *forwarder, int socket);

#endif /* _COMMON_UTILS_T_DEFS_H_ */
