#include<stdio.h>
#include<stdint.h>

int f_read(char *calibF_fname, int nb_antM, int nb_freq, int16_t (*calibF_mtx)[nb_freq*2]){

  FILE *calibF_fd;
  int i,j,l,calibF_e;
  
  calibF_fd = fopen(calibF_fname,"r");
 
  if (calibF_fd) {
    printf("Loading Calibration matrix from %s\n", calibF_fname);
  
    for(i=0;i<nb_antM;i++){
      for(j=0;j<nb_freq*2;j++){
	fscanf(calibF_fd, "%d", &calibF_e);
        calibF_mtx[i][j] = (int16_t)calibF_e;
      }
    }
    printf("%d\n",(int)calibF_mtx[0][0]);
    printf("%d\n",(int)calibF_mtx[1][599]);
  } else
   printf("%s not found, running with defaults\n",calibF_fname);
}
