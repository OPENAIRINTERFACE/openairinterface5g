#ifndef _COMMON_UTILS_T_TRACER_DEFS_H_
#define _COMMON_UTILS_T_TRACER_DEFS_H_

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

/* T gui functions */
void t_gui_start(void);
void t_gui_set_input_signal(int eNB, int frame, int subframe, int antenna,
    int size, void *buf);

#endif /* _COMMON_UTILS_T_TRACER_DEFS_H_ */
