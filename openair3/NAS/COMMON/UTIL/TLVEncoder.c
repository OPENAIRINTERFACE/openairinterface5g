#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "TLVEncoder.h"

int errorCodeEncoder = 0;

const char *errorCodeStringEncoder[] = {
  "No error",
  "Buffer NULL",
  "Buffer too short",
  "Octet string too long for IEI",
  "Wrong message type",
  "Protocol not supported",
};

void tlv_encode_perror(void)
{
  if (errorCodeEncoder >= 0)
    // No error or TLV_DECODE_ERR_OK
    return;

  printf("error: (%d, %s)\n", errorCodeEncoder, errorCodeStringEncoder[errorCodeEncoder * -1]);
}

