#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "PHY/impl_defs_lte.h"

int f_read(char *calibF_fname, int nb_ant, int nb_freq, int32_t **tdd_calib_coeffs){

  FILE *calibF_fd;
  int i,j,calibF_e;
  
  calibF_fd = fopen(calibF_fname,"r");
 
  if (calibF_fd) {
    printf("Loading Calibration matrix from %s\n", calibF_fname);
  
    for(i=0;i<nb_ant;i++){
      for(j=0;j<nb_freq*2;j++){
        if (fscanf(calibF_fd, "%d", &calibF_e) != 1) abort();
        tdd_calib_coeffs[i][j] = (int16_t)calibF_e;
      }
    }
    printf("%d\n",(int)tdd_calib_coeffs[0][0]);
    printf("%d\n",(int)tdd_calib_coeffs[1][599]);
  } else
   printf("%s not found, running with defaults\n",calibF_fname);
  /* TODO: what to return? is this code used at all? */
  return 0;
}


int estimate_DLCSI_from_ULCSI(int32_t **calib_dl_ch_estimates, int32_t **ul_ch_estimates, int32_t **tdd_calib_coeffs, int nb_ant, int nb_freq) {

  /* TODO: what to return? is this code used at all? */
  return 0;

}

int compute_BF_weights(int32_t **beam_weights, int32_t **calib_dl_ch_estimates, PRECODE_TYPE_t precode_type, int nb_ant, int nb_freq) {
  switch (precode_type) {
  //case MRT
  case 0 :
  //case ZF
  break;
  case 1 :
  //case MMSE
  break;
  case 2 :
  break;
  default :
  break;  
}
  /* TODO: what to return? is this code used at all? */
  return 0;
} 

// temporal test function
/*
void main(){
  // initialization
  // compare
  printf("Hello world!\n");
}
*/
