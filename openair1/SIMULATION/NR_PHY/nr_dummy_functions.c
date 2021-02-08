int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req)             { return(0);  }
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req)                            { return(0);  }
int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req)       { return(0);  }
//int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req)       { return(0);  }
//int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req) { return(0);  }
int32_t get_uldl_offset(int nr_bandP)                                       { return(0);  }
NR_IF_Module_t *NR_IF_Module_init(int Mod_id)                               {return(NULL);}
int dummy_nr_ue_dl_indication(nr_downlink_indication_t *dl_info)            { return(0);  }
int dummy_nr_ue_ul_indication(nr_uplink_indication_t *ul_info)              { return(0);  }
void nr_fill_dl_indication(nr_downlink_indication_t *dl_ind,
                           fapi_nr_dci_indication_t *dci_ind,
                           fapi_nr_rx_indication_t *rx_ind,
                           UE_nr_rxtx_proc_t *proc,
                           PHY_VARS_NR_UE *ue,
                           uint8_t gNB_id) {}
void nr_fill_rx_indication(fapi_nr_rx_indication_t *rx_ind,
                           uint8_t pdu_type,
                           uint8_t gNB_id,
                           PHY_VARS_NR_UE *ue,
                           NR_UE_DLSCH_t *dlsch0,
                           uint16_t n_pdus) {}
