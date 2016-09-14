#ifndef __NAS_CONTROL_H__
#define __NAS_CONTROL_H__

#ifdef NODE_MT
//-----------------------------------------------------------------------------
void nas_ue_control_init (void);
//int nas_ue_DC_attach(void);
//int nas_ue_DC_attach_complete(void);
int  nas_ue_GC_Rcve_FIFO (void);
int  nas_ue_NT_Rcve_FIFO (void);
int  nas_ue_DC_Rcve_FIFO (void);
#endif

#ifdef NODE_RG
//-----------------------------------------------------------------------------
void nas_rg_control_init (void);
//int nas_rg_DC_ConnectMs(void) ;
int  nas_rg_DC_Rcve_FIFO (int time);

int nasrg_rrm_socket_init(void);
void nasrg_rrm_fifos_init (void);
void nasrg_rrm_to_rrc_write (void);
void nasrg_rrm_from_rrc_read (void);

int nasrg_meas_loop (int time, int UE_Id);
void nas_rg_print_buffer (char *buffer, int length);
/*void nasrg_print_meas_report (char *rrc_rrm_meas_payload, uint16_t type);
void nasrg_print_bs_meas_report (char *rrc_rrm_meas_payload, uint16_t type);*/
void nasrg_print_meas_report (char *rrc_rrm_meas_payload, unsigned short type);
void nasrg_print_bs_meas_report (char *rrc_rrm_meas_payload, unsigned short type);
#endif

#endif
