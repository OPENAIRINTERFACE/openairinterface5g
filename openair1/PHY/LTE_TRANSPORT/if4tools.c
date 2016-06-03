/*
typedef struct data_block_type {
    
} data_block;
*/

/*
void allocate_data_block(long *data_block ,int length){
	data_block = malloc(length*sizeof(long));
}
*/

void send_IF4(PHY_VARS_eNB eNB, int subframe){
	eNB_proc_t *proc = &eNB->proc;
	//int frame=proc->frame_tx;
	//int subframe=proc->subframe_tx;

	LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
	int i,j;

	float *data_block = malloc(length*sizeof(long));

	
	// find number of consecutive non zero in values in symbol
	// NrOfNonZeroValues
	
	// how many values does the atan function output?
	
	for(i = 0; i <= fp->symbols_per_tti; i++){
		for(j = 0; j < NrOfNonZeroValues; j=j+2){  
			
			symbol = eNB->common_vars.txdataF[0][0 /*antenna number*/][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)]
			
			data_block[j] = Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j -1])<<16 + Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j]);
			data_block[j+NrOfNonZeroValues] = Atan(subframe[i][j+1])<<16 + Atan(subframe[i][j+2]);
			// use memset?
		}
		
	}

/*
memset(&eNB->common_vars.txdataF[0][aa][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)],
             0,fp->ofdm_symbol_size*(fp->symbols_per_tti)*sizeof(int32_t));
    }
  */  
    
}
