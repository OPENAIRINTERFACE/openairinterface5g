#ifndef _CONFIG_H_
#define _CONFIG_H_

void append_received_config_chunk(char *buf, int length);
void store_config_file(char *filename);
void verify_config(void);

#endif /* _CONFIG_H_ */
