#ifndef _TRACER_DEFS_H_
#define _TRACER_DEFS_H_

void *make_plot(int width, int height, int bufsize);
void plot_set(void *plot, float *data, int len, int pos);
void iq_plot_set(void *plot, short *data, int len, int pos);

#endif /* _TRACER_DEFS_H_ */
