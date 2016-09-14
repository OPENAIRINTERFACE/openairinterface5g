#ifndef S1AP_ENB_MANAGEMENT_PROCEDURES_H_
#define S1AP_ENB_MANAGEMENT_PROCEDURES_H_

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME(
  s1ap_eNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id);

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME_from_instance(s1ap_eNB_instance_t *instance_p);

void s1ap_eNB_remove_mme_desc(s1ap_eNB_instance_t * instance);

void s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p);

s1ap_eNB_instance_t *s1ap_eNB_get_instance(uint8_t mod_id);

uint16_t s1ap_eNB_fetch_add_global_cnx_id(void);

void s1ap_eNB_prepare_internal_data(void);

#endif /* S1AP_ENB_MANAGEMENT_PROCEDURES_H_ */
