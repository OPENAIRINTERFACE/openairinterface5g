#ifndef _CONFIG_H_
#define _CONFIG_H_

void clear_remote_config(void);
void append_received_config_chunk(char *buf, int length);
void load_config_file(char *filename);
void verify_config(void);

#endif /* _CONFIG_H_ */
