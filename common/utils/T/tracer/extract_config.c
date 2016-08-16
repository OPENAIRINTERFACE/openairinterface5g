#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../T_defs.h"

void usage(void)
{
  printf(
"options:\n"
"    -i <input file>           this option is mandatory\n"
  );
  exit(1);
}

#define ERR printf("ERROR: read file %s failed\n", input_filename)

int main(int n, char **v)
{
  char *input_filename = NULL;
  int i;
  FILE *in;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-i"))
      { if (i > n-2) usage(); input_filename = v[++i]; continue; }
    usage();
  }

  if (input_filename == NULL) {
    printf("ERROR: provide an input file (-i)\n");
    exit(1);
  }

  in = fopen(input_filename, "r");
  if (in == NULL) { perror(input_filename); abort(); }

  while (1) {
    int type;
    int32_t length;
    char v[T_BUFFER_MAX];
    int vpos = 0;

    /* read event from file */
    if (fread(&length, 4, 1, in) != 1) break;
    memcpy(v+vpos, &length, 4);
    vpos += 4;
#ifdef T_SEND_TIME
    if (length < sizeof(struct timespec)) { ERR; break; }
    if (fread(v+vpos, sizeof(struct timespec), 1, in) != 1) { ERR; break; }
    vpos += sizeof(struct timespec);
    length -= sizeof(struct timespec);
#endif
    if (length < sizeof(int)) { ERR; break; }
    if (fread(&type, sizeof(int), 1, in) != 1) { ERR; break; }
    memcpy(v+vpos, &type, sizeof(int));
    vpos += sizeof(int);
    length -= sizeof(int);
    if (length) if (fread(v+vpos, length, 1, in) != 1) { ERR; break; }
    vpos += length;

    if (type == -1) {
      if (length < sizeof(int)) { ERR; break; }
      length -= sizeof(int);
      if (fwrite(v+vpos-length, length, 1, stdout) != 1) { ERR; break; }
    }

    /* TODO: parse all file? */
    if (type == -2) break;
  }

  fclose(in);

  return 0;
}
