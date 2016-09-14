#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TLVDecoder.h"

int errorCodeDecoder = 0;

const char *errorCodeStringDecoder[] = {
  "No error",
  "Buffer NULL",
  "Buffer too short",
  "Unexpected IEI",
  "Mandatory field not present",
  "Wrong message type",
  "EXT value doesn't match",
  "Protocol not supported",
};

void tlv_decode_perror(void)
{
  if (errorCodeDecoder >= 0)
    // No error or TLV_DECODE_ERR_OK
    return;

  printf("TLV decoder error: (%d, %s)\n", errorCodeDecoder, errorCodeStringDecoder[errorCodeDecoder * -1]);
}


