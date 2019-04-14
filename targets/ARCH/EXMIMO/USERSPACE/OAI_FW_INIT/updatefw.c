/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
    C Header file <updatefw.h> for updatefw tool.

    K. Khalfallah, Aug, 2007
    kkhalfallah@free.fr
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include "openair_device.h"

#include "elf.h"
#include "elftypes.h"
#define EM_SPARC    2
#define MAX_SIZE_STRING_TABLE    1024
#define MAX_SIZE_SECTION_NAME    32

#include "updatefw.h"

/* Interface with OpenAirInterface driver */
#define DEVICE_NAME "/dev/openair0"
#define ACCESS_MODE O_RDONLY

#define SWITCH_IS_OFF        0
#define SWITCH_IS_ON         1
static unsigned int helpflag = SWITCH_IS_OFF;
static unsigned int pflag = SWITCH_IS_OFF;
static unsigned int noexecflag = SWITCH_IS_OFF;
static unsigned int fflag = SWITCH_IS_OFF;
static unsigned int sflag = SWITCH_IS_OFF;
static unsigned int rebootflag = SWITCH_IS_OFF;
static FILE* p_elfimage;
static Elf32_Ehdr elf_Ehdr;
static Elf32_Shdr elf_Shdr;
long Shdr_pos, StringSec_pos, StringSec_size;
//char section_name[MAX_SIZE_SECTION_NAME];
char SecNameStnTable[MAX_SIZE_STRING_TABLE];
unsigned int ioctl_params[4];

#define LONGOPTIONS_NB 6
struct option updatefw_longopts[LONGOPTIONS_NB+1] = {
  {  .name = "help",        .has_arg = no_argument,       .flag = &helpflag,      .val = SWITCH_IS_ON  },
  {  .name = "firmware",    .has_arg = required_argument, .flag = NULL,           .val = 'f'           },
  {  .name = "pretend",     .has_arg = no_argument,       .flag = &pflag,         .val = SWITCH_IS_ON  },
  {  .name = "noexec",      .has_arg = no_argument,       .flag = &noexecflag,    .val = SWITCH_IS_ON  },
  {  .name = "stackptr",    .has_arg = required_argument, .flag = NULL,           .val = 's'           },
  {  .name = "forcereboot", .has_arg = no_argument,       .flag = &rebootflag,    .val = SWITCH_IS_ON  },
  {0, 0, 0, 0}
};
#define TRUE  1
#define FALSE 0

void show_usage(char* pgname) {
  unsigned int i;
  fprintf(stderr, "  %s : Tool to update firmware of Cardbus-MIMO-1/Leon3 card through the PCI interface,\n", pgname);
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, " that is, even without using the Xilinx parallel cable. The tool performs sequentially\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, " the 5 following operations:\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "       1) Ask card driver to reboot (optionally)\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "       2) Ask card driver to transfer .text section\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "       3) Ask card driver to transfer .data section\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "       4) Ask card driver to clear .bss section\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "       5) Ask card driver to have Leon processor set stack-pointer\n");
  fprintf(stderr, "    ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "          and jump to firmware entry.\n");
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s [-f|--firmware 'filename'] [-s|--stackptr value] [-b|--forcereboot]\n",pgname);
  fprintf(stderr, "   ");

  for (i=0; i<strlen(pgname); i++) fprintf(stderr, " ");

  fprintf(stderr, "[-v|-vv|-vvv|-vvvv] [-h|--help] [-p|--pretend]\n\n");
  fprintf(stderr, "  [-f|--firmware ]     Mandatory option, to set the name of the executable file\n");
  fprintf(stderr, "                       of the firmware to update the remote card with.\n");
  fprintf(stderr, "                       The file should be an executable file in the Sparc32 ELF binary\n");
  fprintf(stderr, "                       format, containing at least the 3 mandatory sections .text, .data & .bss\n");
  fprintf(stderr, "  [-s|--stackptr]      Mandatory option, to set the value of the stack pointer before\n");
  fprintf(stderr, "                       giving hand to the firmware. The specified value must be an integer\n");
  fprintf(stderr, "                       number of 8 hexadecimal digits preceded by token '0x'.\n");
  fprintf(stderr, "  [-b|--forcereboot]   Force reboot of Leon processor before updating the firmware.\n");
  fprintf(stderr, "  [-p|--pretend]       Just pretend to transfer commands (ioctls) to driver, don't do it\n");
  fprintf(stderr, "                       actually. Useful together with verbose -vv switch.\n");
  fprintf(stderr, "  [-n|--noexec]        Transfer all data and text segments, but don't do actually start the code on Leon (no START_EXEC).\n");
  fprintf(stderr, "  [-v|-vv|-vvv|-vvvv]  Verbose modes.\n");
  fprintf(stderr, "  [-h|--help]          Displays this help.\n");
}

int get_elf_header(Elf32_Ehdr* p_Elf32_hdr, FILE* p_file) {
  int nbread;
  nbread = fread(p_Elf32_hdr, sizeof(elf_Ehdr), 1, p_file);

  if (nbread != 1) {
    fprintf(stderr, "Error : while reading ELF header (fread() returned %d)\n", nbread);
    exit(-1);
  }

  invert2(p_Elf32_hdr->e_type);            /* $$$$KMK: bad code !!! Endianess dependant ! */
  invert2(p_Elf32_hdr->e_machine);
  invert4(p_Elf32_hdr->e_version);
  invert4(p_Elf32_hdr->e_entry);
  invert4(p_Elf32_hdr->e_phoff);
  invert4(p_Elf32_hdr->e_shoff);
  invert4(p_Elf32_hdr->e_flags);
  invert2(p_Elf32_hdr->e_ehsize);
  invert2(p_Elf32_hdr->e_phentsize);
  invert2(p_Elf32_hdr->e_phnum);
  invert2(p_Elf32_hdr->e_shentsize);
  invert2(p_Elf32_hdr->e_shnum);
  invert2(p_Elf32_hdr->e_shstrndx);
  return (p_Elf32_hdr->e_type == ET_EXEC)
         && (p_Elf32_hdr->e_machine == EM_SPARC)
         && (p_Elf32_hdr->e_version == EV_CURRENT)
         && (p_Elf32_hdr->e_ident[EI_CLASS] == ELFCLASS32)
         && (p_Elf32_hdr->e_ident[EI_DATA] == ELFDATA2MSB)
         && (p_Elf32_hdr->e_ident[EI_VERSION] == EV_CURRENT);
}

int get_elf_section_header(Elf32_Shdr* p_Elf32_Shdr, FILE* p_file, unsigned int section_ndx) {
  int nbread;
  /* retrieve the position of the section header table */
  fseek(p_file, Shdr_pos + (section_ndx * elf_Ehdr.e_shentsize), 0);
  /* set file pointer to the position of the section header section_ndx */
  // seekrst = fseek(p_file, (long), 0);
  nbread = fread(p_Elf32_Shdr, sizeof(elf_Shdr), 1, p_file);

  if (nbread != 1) {
    fprintf(stderr, "Error : while reading elf section header (fread() returned %d)\n", nbread);
    exit(-1);
  }

  invert4(p_Elf32_Shdr->sh_name);            /* $$$$KMK: bad code !!! Endianess dependant ! */
  invert4(p_Elf32_Shdr->sh_type);
  invert4(p_Elf32_Shdr->sh_flags);
  invert4(p_Elf32_Shdr->sh_addr);
  invert4(p_Elf32_Shdr->sh_offset);
  invert4(p_Elf32_Shdr->sh_size);
  invert4(p_Elf32_Shdr->sh_link);
  invert4(p_Elf32_Shdr->sh_info);
  invert4(p_Elf32_Shdr->sh_addralign);
  invert4(p_Elf32_Shdr->sh_entsize);
  //fseek(p_file, StringSec_pos + p_Elf32_Shdr->sh_name, 0);
  //fread();
  //strcpy(sname, );
  return nbread;
}

void find_and_transfer_section(char* section_name, unsigned int verboselevel) {
  /* Interface with driver */
  int ioctlretval;
  int ifile;
  char* section_content;
  int nbread;
  unsigned int secnb = 0;

  if(p_elfimage == NULL) {
    fprintf(stderr, "%s %d:Error, firmware file not opened\n",__FILE__,__LINE__);
  }
  for (secnb = 0 ; secnb < elf_Ehdr.e_shnum; secnb++) {
    get_elf_section_header(&elf_Shdr, p_elfimage, secnb);

    if (!strcmp(SecNameStnTable + elf_Shdr.sh_name, section_name)) {
      if (verboselevel >= VERBOSE_LEVEL_SECTION_DETAILS)
        printf("Info: ok, found section %s (as section nb. %u)\n", SecNameStnTable + elf_Shdr.sh_name, secnb);

      /* Check that section size is a multiple of 4 bytes. */
      if (elf_Shdr.sh_size % 4) {
        fprintf(stderr, "Error: section %s has a non-multiple-of-4-bytes size (%lu).\n",
                SecNameStnTable + elf_Shdr.sh_name, (unsigned long)elf_Shdr.sh_size);
        fclose(p_elfimage);
        exit(-1);
      } else if (verboselevel >= VERBOSE_LEVEL_SECTION_DETAILS) {
        printf("Info: ok, section %s has size %lu bytes (multiple of 4 bytes).\n",
               SecNameStnTable + elf_Shdr.sh_name, (unsigned long)elf_Shdr.sh_size);
      }

      /* Dynamically allocate a chunk of memory to store the section into. */
      section_content = (char*)malloc(elf_Shdr.sh_size);

      /* Fail if the address returned by malloc does not fit in 32 bits.
       * The kernel driver gets this address as 32 bits and uses it to copy
       * from userspace. If the address does not fit into 32 bits the kernel
       * will copy garbage or fail to copy completely.
       * To be reworked if things do not work on some setups.
       */
      if (sizeof(char*) > 4 && (((uintptr_t)section_content)>>32)) {
        fprintf(stderr, "FATAL ERROR: an internal serious problem has been encountered.\n");
        fprintf(stderr, "Contact the authors so as to solve this issue.\n");
        exit(-1);
      }

      if (!section_content) {
        fprintf(stderr, "Error: could not dynamically allocate %lu bytes for section %s.\n",
                (unsigned long)elf_Shdr.sh_size, (SecNameStnTable + elf_Shdr.sh_name));
        fclose(p_elfimage);
        exit(-1);
      } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
        printf("Info: ok, dynamically allocated a %lu bytes buffer for section %s.\n",
               (unsigned long)elf_Shdr.sh_size, SecNameStnTable + elf_Shdr.sh_name);
      }

      /* Position the file cursor at the begining of proper section. */
      fseek(p_elfimage, (long)(elf_Shdr.sh_offset), 0);
      /* Copy the section's content into this temporary buffer. */
      nbread = fread(section_content, elf_Shdr.sh_size, 1, p_elfimage);

      if (nbread != 1) {
        fprintf(stderr, "Error: could not read %lu bytes from ELF file into dynamic buffer.\n", (unsigned long)elf_Shdr.sh_size);
        free(section_content);
        fclose(p_elfimage);
        exit(-1);
      } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
        printf("Info: ok, copied content of section %s into dynamic buffer (%lu bytes copied).\n",
               SecNameStnTable + elf_Shdr.sh_name, (unsigned long)elf_Shdr.sh_size);
      }

      /* Open the special device file. */
      if (!pflag) {
        ifile = open(DEVICE_NAME, ACCESS_MODE, 0);

        if (ifile<0) {
          fprintf(stderr, "Error: could not open %s (open() returned %d, errno=%u)\n", DEVICE_NAME, ifile, errno);
          free(section_content);
          fclose(p_elfimage);
          exit(-1);
        } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
          printf("Info: ok, successfully opened %s.\n", DEVICE_NAME);
        }

        /* Collect control data for ioctl. */
        ioctl_params[0] = UPDATE_FIRMWARE_TRANSFER_BLOCK;
        ioctl_params[1] = elf_Shdr.sh_addr;
        ioctl_params[2] = elf_Shdr.sh_size / 4;
        /* This is wrong on x86_64 because a pointer is 64 bits and
         * an unsigned int is only 32 bits.
         * The compiler emits a warning, but the test
         * above on the value of section_content makes it work
         * (hopefully).
         * To be changed if things do not work.
         */
        ioctl_params[3] = (unsigned int)((unsigned int*)section_content);
        //invert4(ioctl_params[1]);
        //invert4(ioctl_params[2]);
        /* Call ioctl driver */
        ioctlretval = ioctl(ifile, openair_UPDATE_FIRMWARE, ioctl_params);

        if (ioctlretval) {
          fprintf(stderr, "Error: ioctl on %s failed.\n", DEVICE_NAME);
          close(ifile);
          free(section_content);
          fclose(p_elfimage);
          exit(-1);
        } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
          printf("Info: ok, successful ioctl on %s for section %s.\n",
                 DEVICE_NAME, SecNameStnTable + elf_Shdr.sh_name);
        }

        close(ifile);
      } /* pflag */

      free(section_content);
    } /* section_name */
  } /* for secnb */
}

void find_and_clear_section_bss(unsigned int verboselevel) {
  /* Interface with driver */
  int ioctlretval;
  int ifile;
  unsigned int secnb = 0;

  if(p_elfimage == NULL) {
    fprintf(stderr, "%s %d:Error, firmware file not opened\n",__FILE__,__LINE__);
  }
  for (secnb = 0 ; secnb < elf_Ehdr.e_shnum; secnb++) {
    get_elf_section_header(&elf_Shdr, p_elfimage, secnb);

    if (!strcmp(SecNameStnTable + elf_Shdr.sh_name, ".bss")) {
      if (verboselevel >= VERBOSE_LEVEL_SECTION_DETAILS)
        printf("Info: ok, found section %s (as section nb. %u)\n", SecNameStnTable + elf_Shdr.sh_name, secnb);

      /* Open the special device file. */
      if (!pflag) {
        ifile = open(DEVICE_NAME, ACCESS_MODE, 0);

        if (ifile<0) {
          fprintf(stderr, "Error: could not open %s (open() returned %d, errno=%u)\n", DEVICE_NAME, ifile, errno);
          fclose(p_elfimage);
          exit(-1);
        } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
          printf("Info: ok, successfully opened %s.\n", DEVICE_NAME);
        }

        /* Collect control data for ioctl. */
        ioctl_params[0] = UPDATE_FIRMWARE_CLEAR_BSS;
        ioctl_params[1] = elf_Shdr.sh_addr;
        ioctl_params[2] = elf_Shdr.sh_size;
        //invert4(ioctl_params[1]);
        //invert4(ioctl_params[2]);
        /* Call ioctl driver */
        ioctlretval = ioctl(ifile, openair_UPDATE_FIRMWARE, ioctl_params);

        if (ioctlretval) {
          fprintf(stderr, "Error: ioctl on %s failed.\n", DEVICE_NAME);
          close(ifile);
          fclose(p_elfimage);
          exit(-1);
        } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
          printf("Info: ok, successful ioctl on %s for section %s.\n",
                 DEVICE_NAME, SecNameStnTable + elf_Shdr.sh_name);
        }

        close(ifile);
      } /* pflag */
    } /* section_name */
  } /* for secnb */
}

int main(int argc, char** argv) {
  /* Interface with driver */
  int ioctlretval;
  int ifile;
  /* Interface with getopt_long() libC function */
  int getoptret;
  int indexptr;
  int erroroption = FALSE;
  char* p_str_fwn = NULL;
  char* p_str_stkptr = NULL;
  char* filename;
  unsigned int verboselevel = 0;
  unsigned int i;
  char* c;
  unsigned int stackpointer = 0;
  unsigned int ioctl_test_gok = 0;
  unsigned int do_UPDATE_FIRMWARE_START_EXECUTION = 1;

  /*****************
   * Parse options *
   *****************/
  while ((getoptret = getopt_long_only (argc, argv, "f:vhpns:b", updatefw_longopts, &indexptr)) != -1)
    switch (getoptret) {
      /* Without-argument options */
    case 0: /* means that the option just sets a flag. Nothing has to be done,
                 since getopt_long already sets the flag. */
      break;

      /* With-argument options & equivalent short options */
    case 'v':
      verboselevel++;
      break;

    case 'h': /* short switch -h was used */
      helpflag = SWITCH_IS_ON;
      break;

    case 'p': /* short switch -p was used */
      pflag = SWITCH_IS_ON;
      break;

    case 'n': /* short switch -n was used */
      noexecflag = SWITCH_IS_ON;
      break;

    case 'f': /* short switch -f was used */
      fflag = SWITCH_IS_ON;
      p_str_fwn = optarg;
      break;

    case 's': /* short switch -s was used */
      sflag = SWITCH_IS_ON;
      p_str_stkptr = optarg;
      break;

    case 'b': /* short switch -b was used */
      rebootflag = SWITCH_IS_ON;
      break;

    default:
      erroroption = TRUE;
      break;
    };

  /* End of while */

  /********************************
   * Check consistency of options *
   ********************************/
  /* First, any irregularity in the use of the options ? Leave. */
  if (erroroption == TRUE) {
    fprintf(stderr, "%s: Misuse (--help to show usage)\n", argv[0]);
    exit(-1);
  }

  /* Let print the help if it has been explicitly asked for */
  if (helpflag == SWITCH_IS_ON) {
    show_usage(argv[0]);
    exit(0);
  }

  /* Check that mandatory firmware option was specified */
  if (fflag != SWITCH_IS_ON) {
    fprintf(stderr, "%s: missing mandatory [-f|--firmware] option (--help to show usage).\n", argv[0]);
    exit(-1);
  }

  /* Check that mandatory stack pointer option was specified */
  if (sflag != SWITCH_IS_ON) {
    fprintf(stderr, "%s: missing mandatory [-s|--stackptr] option (--help to show usage).\n", argv[0]);
    exit(-1);
  }

  /* Check consistency of stack pointer value. */
  /* 1/2 - The address must start with the 2 characters '0x' and be exactly 10 characters long. */
  if ((strncmp(p_str_stkptr, "0x", 2)) || (strlen(p_str_stkptr) != 10)) { /* 10 is for 0x + 8 hex digits. */
    fprintf(stderr, "%s: Invalid value to [s|--stackptr] option (--help to show usage).\n", argv[0]);
    exit(-1);
  }

  /* 2/2 - The address must only be made of hexadecimal digits. */
  for (i=0,c=p_str_stkptr+2; i < 8 ; i++,c++) {
    if (!
        (   ((*c >= '0') && (*c <= '9'))
            || ((*c >= 'a') && (*c <= 'f'))
            || ((*c >= 'A') && (*c <= 'F'))
        )
       ) {
      fprintf(stderr, "%s: Invalid value to [s|--stackptr] option (--help to show usage).\n", argv[0]);
      exit(-1);
    }

    /* Transcode string hexadecinmal value into binary value */
    if ((*c <= '9') && (*c >= '0'))
      stackpointer |= (*c - '0') << 28-(i*4);
    else if ((*c <= 'F') && (*c >= 'A'))
      stackpointer |= ((*c - 'A') + 10) << 28-(i*4);
    else if ((*c <= 'f') && (*c >= 'a'))
      stackpointer |= ((*c - 'a') + 10) << 28-(i*4);
  }

  /* Open firmware file in READ_ONLY mode. */
  filename = p_str_fwn;
  if (filename == NULL) {
     fprintf(stderr, "%s %d: No filename specified\n", __FILE__, __LINE__);
     exit(-1);
  }
  p_elfimage = fopen(filename, READ_FILE_MODE);

  if (p_elfimage == NULL) {
    fprintf(stderr, "Error : could not open file <%s> in %s mode.\n", filename, READ_FILE_MODE);
    exit(-1);
  }

  if (verboselevel >= VERBOSE_LEVEL_ACCESS_FILES)
    printf("Info: using file <%s> (successfully opened in %s mode).\n", filename, READ_FILE_MODE);

  /* Read elf binary image file */
  /* Get informations from header file */
  if (!(get_elf_header(&elf_Ehdr, p_elfimage))) {
    fprintf(stderr, "Error : file doesn't match expected format.\n");
    fclose(p_elfimage);
    exit(-1);
  }

  if (verboselevel >= VERBOSE_LEVEL_ACCESS_FILES)
    printf("Info: ok, file matches expected format.\n");

  /* record the position of the section header table in the file */
  fseek(p_elfimage, (long)(elf_Ehdr.e_shoff), 0);
  Shdr_pos = ftell(p_elfimage);
  /* record the position of the section name string table */
  get_elf_section_header(&elf_Shdr, p_elfimage, elf_Ehdr.e_shstrndx);
  fseek(p_elfimage, (long)(elf_Shdr.sh_offset), 0);
  StringSec_pos = ftell(p_elfimage);
  StringSec_size = elf_Shdr.sh_size;

  /* copy the string table into global variable */
  if (StringSec_size > MAX_SIZE_STRING_TABLE) {
    fprintf(stderr, "Error: section name string table too big (%ld > %d);"
            " increase max. allowed value in source code and recompile\n",
            StringSec_size, MAX_SIZE_STRING_TABLE);
    fclose(p_elfimage);
    exit(-1);
  }

  fread(SecNameStnTable, 1, StringSec_size, p_elfimage);

  /* Action 1: force reboot (if option was specified) */
  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) {
    if (rebootflag == SWITCH_IS_ON)
      printf("Info: entering action #1 (forcing reboot of Leon).\n");
    else
      printf("Info: squeezed action #1 (forcing reboot of Leon).\n");
  }

  if ((rebootflag == SWITCH_IS_ON) && (!pflag)) {
    ifile = open(DEVICE_NAME, ACCESS_MODE, 0);

    if (ifile<0) {
      fprintf(stderr, "Error: could not open %s (open() returned %d, errno=%u).\n", DEVICE_NAME, ifile, errno);
      fclose(p_elfimage);
      exit(-1);
    } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
      printf("Info: ok, successfully opened %s.\n", DEVICE_NAME);
    }

    ioctl_params[0] = UPDATE_FIRMWARE_FORCE_REBOOT;
    /* Call ioctl driver */
    ioctlretval = ioctl(ifile, openair_UPDATE_FIRMWARE, ioctl_params);

    if (ioctlretval) {
      fprintf(stderr, "Error: ioctl on %s failed.\n", DEVICE_NAME);
      close(ifile);
      fclose(p_elfimage);
      exit(-1);
    } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
      printf("Info: ok, successful ioctl on %s to force reboot.\n", DEVICE_NAME);
    }

    /* If the card received the boot order, then
         1) The firmware who handled the order must have cleared the BOOT_GOK bit
            before rebooting ;
         2) Boot-strap should once again set it back upon its init operations.
       So we wait for this bit to be set again before issuing the rest of the
       update-firmware operations (transfer loadable sections and so on...).
       Note: putting the polling loop here is better than to have it in the
       openair driver, because it avoids hanging the kernel if the card was
       ever not to reboot cleanly. If the loop was to not end, user can simply
       gives up the update-firmware operation by issuing a Ctrl-C. */
    ioctl_params[0] = UPDATE_FIRMWARE_TEST_GOK;

    /* Call ioctl driver */
    if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS)
      printf("Info: waiting for GOK bit (as acknowledge of reboot command) - May take a few seconds...\n");

    ioctl_test_gok = 0;

    do {
      ioctlretval = ioctl(ifile, openair_UPDATE_FIRMWARE, ioctl_params);
      ioctl_test_gok++;
      sleep(1); /* sleep 1 second */

      if (!(ioctl_test_gok % 10))
        printf("Warning: card does not seem to have rebooted cleanly (no GOK bit). Still trying ([Ctrl-C] to abort).\n");
    } while (ioctlretval);

    if (verboselevel >= VERBOSE_LEVEL_IOCTL)
      printf("Info: ok, card set again GOK bit (after %u seconds).\n", ioctl_test_gok);

    close(ifile);
  } /* rebootflag && pflag */

  if ((verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) && (rebootflag == SWITCH_IS_ON))
    printf("Info: action #1 done.\n");

  /* Action 2: Find the .text section */
  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: entering action #2 (Transfer .text section).\n");

  find_and_transfer_section(".text", verboselevel);
  sleep(1);

  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: action #2 done.\n");

  /* Action 3 : Find the .data section */
  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: entering action #3 (Transfer .data section).\n");

  find_and_transfer_section(".data", verboselevel);
  sleep(1);

  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: action #3 done.\n");

  /* Action 4 : Find the .bss  section */
  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: entering action #4 (Clear .bss section).\n");

  find_and_clear_section_bss(verboselevel);
  sleep(1);

  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: action #4 done.\n");

  /* Action 5 : Find the entry address */
  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: entering action #5 (Jump to firmware/set stack-pointer).\n");

  if (verboselevel >= VERBOSE_LEVEL_SECTION_DETAILS)
    printf("Info: Firmware entry point = 0x%08x, setting stack pointer = 0x%08x.\n", (unsigned int)elf_Ehdr.e_entry, stackpointer);

  /* Open the special device file. */
  if (!pflag) {
    ifile = open(DEVICE_NAME, ACCESS_MODE, 0);

    if (ifile<0) {
      fprintf(stderr, "Error: could not open %s (open() returned %d, errno=%u).\n", DEVICE_NAME, ifile, errno);
      fclose(p_elfimage);
      exit(-1);
    } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
      printf("Info: ok, successfully opened %s.\n", DEVICE_NAME);
    }

    /* Collect control data for ioctl. */
    ioctl_params[0] = UPDATE_FIRMWARE_START_EXECUTION;
    ioctl_params[1] = elf_Ehdr.e_entry;
    ioctl_params[2] = stackpointer;

    //invert4(ioctl_params[1]);
    //invert4(ioctl_params[2]);
    /* Call ioctl driver */
    if ( noexecflag == SWITCH_IS_OFF ) {
      ioctlretval = ioctl(ifile, openair_UPDATE_FIRMWARE, ioctl_params);

      if (ioctlretval) {
        fprintf(stderr, "Error: ioctl on %s failed.\n", DEVICE_NAME);
        close(ifile);
        fclose(p_elfimage);
        exit(-1);
      } else if (verboselevel >= VERBOSE_LEVEL_IOCTL) {
        printf("Info: ok, successful ioctl on %s for stack-pointer setting/firmware-execution.\n", DEVICE_NAME);
      }
    }

    close(ifile);
  } /* pflag */

  if (verboselevel >= VERBOSE_LEVEL_MAIN_STEPS) printf("Info: action #5 done.\n");

  fclose(p_elfimage);

  /* Did we pretend ? */
  if (pflag) printf("Nothing done.\n");

  exit(0);
}
