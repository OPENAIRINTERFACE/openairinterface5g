#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "TrackingAreaUpdateComplete.h"

int decode_tracking_area_update_complete(tracking_area_update_complete_msg *tracking_area_update_complete, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, TRACKING_AREA_UPDATE_COMPLETE_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  return decoded;
}

int encode_tracking_area_update_complete(tracking_area_update_complete_msg *tracking_area_update_complete, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, TRACKING_AREA_UPDATE_COMPLETE_MINIMUM_LENGTH, len);

  return encoded;
}

