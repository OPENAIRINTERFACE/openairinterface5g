#ifndef __NR_LDPC_DECODER_LYC__H__
#define __NR_LDPC_DECODER_LYC__H__


/*! \file PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_decoder_LYC.h
 * \brief LDPC cuda support BG1 all length
 * \author NCTU OpinConnect Terng-Yin Hsu,WEI-YING,LIN
 * \email tyhsu@cs.nctu.edu.tw
 * \date 13-05-2020
 * \version 
 * \note
 * \warning
 */
 

/***
   \brief LDPC decoder
   \param p_decParams LDPC decoder parameters
   \param p_llr Input LLRs
   \param p_llrOut Output vector
   \param p_profiler LDPC profiler statistics
****/

int32_t nrLDPC_decoder_LYC(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, int block_length, time_stats_t *time_decoder);


void init_LLR_DMA_for_CUDA(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, int block_length);

void warmup_for_GPU(void);

void set_compact_BG(int Zc, short BG);


#endif
