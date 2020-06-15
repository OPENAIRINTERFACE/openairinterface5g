#ifndef __COMMON_UTILS_T_TRACER_CONFIG__H__
#define __COMMON_UTILS_T_TRACER_CONFIG__H__

void clear_remote_config(void);
void append_received_config_chunk(char *buf, int length);
void load_config_file(char *filename);
void verify_config(void);
void get_local_config(char **txt, int *len);

#endif /* __COMMON_UTILS_T_TRACER_CONFIG__H__ */
