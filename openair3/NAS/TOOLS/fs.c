#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "user_api.h"
#include "utils.h"

bool file_exist_and_is_readable(const char *filename) {
    FILE *file ;
    file = fopen(filename, "r");
    if ( file == NULL )
        return false;
    fclose(file);
    return true;
}

char *get_ue_filename(const char *output_dir, int user_id) {
    return make_filename(output_dir, USER_NVRAM_FILENAME, user_id);
}

char *get_emm_filename(const char *output_dir, int user_id) {
    return make_filename(output_dir, EMM_NVRAM_FILENAME, user_id);
}

char *get_usim_filename(const char *output_dir, int user_id) {
	return make_filename(output_dir, USIM_API_NVRAM_FILENAME, user_id);
}

char *make_filename(const char *output_dir, const char *filename, int ueid) {
	size_t size;
    char *str_ueid, *str;

    str_ueid = itoa(ueid);

    if (str_ueid == NULL) {
        perror("ERROR\t: itoa() failed");
        exit(EXIT_FAILURE);
    }

    size = strlen(output_dir)+strlen(filename) + sizeof(ueid) + 1 + 1; // for \0 and for '/'
    str = malloc(size);
    if (str == NULL) {
        perror("ERROR\t: make_filename() failed");
        exit(EXIT_FAILURE);
    }

    snprintf(str, size, "%s/%s%s",output_dir, filename, str_ueid);
    free(str_ueid);

 return str;
}
