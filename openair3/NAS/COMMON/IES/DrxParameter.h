#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef DRX_PARAMETER_H_
#define DRX_PARAMETER_H_

#define DRX_PARAMETER_MINIMUM_LENGTH 3
#define DRX_PARAMETER_MAXIMUM_LENGTH 3

typedef struct DrxParameter_tag {
  uint8_t  splitpgcyclecode;
  uint8_t  cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode:4;
  uint8_t  splitonccch:1;
  uint8_t  nondrxtimer:3;
} DrxParameter;

int encode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_drx_parameter_xml(DrxParameter *drxparameter, uint8_t iei);

int decode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* DRX PARAMETER_H_ */

