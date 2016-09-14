#include <stdint.h>
#include <assert.h>

#ifndef OCTET_STRING_H_
#define OCTET_STRING_H_

typedef struct OctetString_tag {
  uint32_t  length;
  uint8_t  *value;
} OctetString;
#define FREE_OCTET_STRING(oCTETsTRING)                     \
    do {                                                   \
        if ((oCTETsTRING).value != NULL) {                 \
            free((oCTETsTRING).value);                     \
            (oCTETsTRING).value = NULL;                    \
        }                                                  \
        (oCTETsTRING).length = 0;                          \
    } while (0);


#define DUP_OCTET_STRING(oCTETsTRINGoRIG,oCTETsTRINGcOPY)                   \
    do {                                                                    \
        if ((oCTETsTRINGoRIG).value == NULL) {                              \
            (oCTETsTRINGcOPY).length = 0;                                   \
            (oCTETsTRINGcOPY).value = NULL;                                 \
            break;                                                          \
        }                                                                   \
        (oCTETsTRINGcOPY).length = (oCTETsTRINGoRIG).length;                 \
        (oCTETsTRINGcOPY).value  = malloc((oCTETsTRINGoRIG).length+1);      \
        (oCTETsTRINGcOPY).value[(oCTETsTRINGoRIG).length] = '\0';           \
        memcpy((oCTETsTRINGcOPY).value,                                     \
            (oCTETsTRINGoRIG).value,                                        \
            (oCTETsTRINGoRIG).length);                                      \
    } while (0);

OctetString* dup_octet_string(OctetString*octetstring);

void free_octet_string(OctetString *octetstring);

int encode_octet_string(OctetString *octetstring, uint8_t *buffer, uint32_t len);

int decode_octet_string(OctetString *octetstring, uint16_t pdulen, uint8_t *buffer, uint32_t buflen);

char* dump_octet_string_xml(const OctetString * const octetstring);

char* dump_octet_string(const OctetString * const octetstring);

#endif /* OCTET_STRING_H_ */

