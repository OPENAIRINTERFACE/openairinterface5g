/*****************************************************************************

Source      mcc_mnc.h

Version     0.1

Date        {2014/10/02

Product

Subsystem

Author      Lionel GAUTHIER

Description Defines the MCC/MNC list delivered by the ITU

*****************************************************************************/
#ifndef __MCC_MNC_H__
#define __MCC_MNC_H__


typedef struct mcc_mnc_list_s {
  uint16_t mcc;
  char     mnc[4];
} mcc_mnc_list_t;

int find_mnc_length(const char mcc_digit1P,
                    const char mcc_digit2P,
                    const char mcc_digit3P,
                    const char mnc_digit1P,
                    const char mnc_digit2P,
                    const char mnc_digit3P);
#endif
