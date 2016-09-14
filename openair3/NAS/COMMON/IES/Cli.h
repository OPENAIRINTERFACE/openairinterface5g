#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef CLI_H_
#define CLI_H_

#define CLI_MINIMUM_LENGTH 3
#define CLI_MAXIMUM_LENGTH 14

typedef struct Cli_tag {
  OctetString clivalue;
} Cli;

int encode_cli(Cli *cli, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_cli(Cli *cli, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_cli_xml(Cli *cli, uint8_t iei);

#endif /* CLI_H_ */

