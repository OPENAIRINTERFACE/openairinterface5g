#ifndef ENUM_STRING_H_
#define ENUM_STRING_H_

typedef struct {
  int   enum_value;
  char *enum_value_name;
} enum_to_string_t;

extern enum_to_string_t network_access_mode_to_string[NAM_MAX];
extern enum_to_string_t rat_to_string[NUMBER_OF_RAT_TYPE];
extern enum_to_string_t pdn_type_to_string[IP_MAX];

char *enum_to_string(int enum_val, enum_to_string_t *string_table, int nb_element);
#define ACCESS_MODE_TO_STRING(vAL)                          \
    enum_to_string((int)vAL, network_access_mode_to_string, \
    sizeof(network_access_mode_to_string) / sizeof(enum_to_string_t))
#define PDN_TYPE_TO_STRING(vAL)                             \
    enum_to_string((int)vAL, pdn_type_to_string,            \
    sizeof(pdn_type_to_string) / sizeof(enum_to_string_t))

#endif /* ENUM_STRING_H_ */
