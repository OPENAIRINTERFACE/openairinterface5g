/*============================================================================
  File: elftypes.h
 Include file for Elf types. Define elf types for Elf structures.
  Copyright Hewlett-Packard Co. 1995.  All rights reserved.
============================================================================*/

#ifndef ELFTYPES_INCLUDED
#define ELFTYPES_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LP64__
typedef unsigned int  Elf32_Addr; /* Unsigned program address */
typedef unsigned int  Elf32_Off;  /* Unsigned file offset */
typedef    int  Elf32_Sword;  /* Signed large integer */
typedef unsigned int  Elf32_Xword;    /* Unsigned large integer */
#else
typedef unsigned long Elf32_Addr; /* Unsigned program address */
typedef unsigned long Elf32_Off;  /* Unsigned file offset */
typedef    long Elf32_Sword;  /* Signed large integer */
typedef unsigned long   Elf32_Xword;    /* Unsigned large integer */
#endif

typedef unsigned int  Elf32_Word; /* Unsigned large integer */
typedef unsigned short  Elf32_Half; /* Unsigned medium integer */
typedef unsigned char Elf32_Char; /* Unsigned tiny integer */
typedef unsigned char Elf32_Byte; /* Unsigned tiny integer */

#ifdef __LP64__
typedef unsigned long   Elf64_Addr;
typedef unsigned long     Elf64_Off;
typedef unsigned long   Elf64_Xword;
typedef    long   Elf64_Sxword;
#else
typedef unsigned long long  Elf64_Addr;
typedef unsigned long long  Elf64_Off;
typedef unsigned long long  Elf64_Xword;
typedef    long long  Elf64_Sxword;
#endif

typedef unsigned short  Elf64_Half;
typedef    int  Elf64_Sword;
typedef unsigned int  Elf64_Word;
typedef unsigned char Elf64_Byte; /* Unsigned tiny integer */

#ifdef __cplusplus
}
#endif

#endif /* ELFTYPES_INCLUDED */
