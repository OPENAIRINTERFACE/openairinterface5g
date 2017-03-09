#ifndef _DISPLAY_H
#define _DISPLAY_H

#define DISPLAY_EMM 1
#define DISPLAY_USIM 2
#define DISPLAY_UEDATA 4
#define DISPLAY_ALL 7

// return number of files displayed
int display_data_from_directory(const char *directory, int flags);

void display_ue_data(const char *filename);
void display_emm_data(const char *filename);
void display_usim_data(const char *filename);

#endif
