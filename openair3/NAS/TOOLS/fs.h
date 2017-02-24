#ifndef _FS_H
#define _FS_H

#include <stdbool.h>

bool file_exist_and_is_readable(const char *filename);
char *get_ue_filename(const char *output_dir, int user_id);
char *get_emm_filename(const char *output_dir, int user_id);
char *get_usim_filename(const char *output_dir, int user_id);
char *make_filename(const char *output_dir, const char *filename, int ueid);

#endif
