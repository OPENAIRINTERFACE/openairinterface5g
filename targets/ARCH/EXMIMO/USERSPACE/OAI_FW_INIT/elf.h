/*============================================================================
  File: elf.h
       Include file for ELF file.  Defines both 32-/64-bit files.
  Copyright Hewlett-Packard Co. 1996.  All rights reserved.
============================================================================*/

#ifndef ELF_INCLUDED
#define ELF_INCLUDED

#include "elftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
   Constant values for the sizes of fundamental types
============================================================================*/

#define ELF64_FSZ_ADDR    8
#define ELF64_FSZ_HALF    2
#define ELF64_FSZ_OFF   8
#define ELF64_FSZ_SWORD   4
#define ELF64_FSZ_WORD    4
#define ELF64_FSZ_SXWORD  8
#define ELF64_FSZ_XWORD   8

#define ELF32_FSZ_ADDR    4
#define ELF32_FSZ_HALF    2
#define ELF32_FSZ_OFF     4
#define ELF32_FSZ_SWORD   4
#define ELF32_FSZ_WORD    4
#define ELF32_FSZ_XWORD   4

/*============================================================================
   The EI_* macros define indices into the e_ident[] array of the
   ElfXX_Ehdr structure.
============================================================================*/

#define EI_MAG0   0 /* File identification */
#define EI_MAG1   1 /* File identification */
#define EI_MAG2   2 /* File identification */
#define EI_MAG3   3 /* File identification */
#define EI_CLASS  4 /* File class */
#define EI_DATA   5 /* Data encoding */
#define EI_VERSION  6 /* File version */
#define EI_OSABI  7 /* OS/ABI identification */
#define EI_ABIVERSION   8 /* ABI version */
#define EI_PAD    9 /* Start of padding bytes */

#define EI_NIDENT 16 /* Size of e_ident[] */


/*============================================================================
   ElfXX_Ehdr - ELF file header structure.
============================================================================*/

typedef struct {
  unsigned char  e_ident[EI_NIDENT]; /* ELF identification */
  Elf64_Half   e_type;   /* Object file type */
  Elf64_Half   e_machine;  /* Machine type */
  Elf64_Word   e_version;  /* Object file version */
  Elf64_Addr   e_entry;  /* Entry point address */
  Elf64_Off    e_phoff;  /* Program header offset */
  Elf64_Off    e_shoff;  /* Section header offset */
  Elf64_Word   e_flags;  /* Processor-specific flags */
  Elf64_Half   e_ehsize; /* ELF header size */
  Elf64_Half   e_phentsize;  /* Size of program header entry */
  Elf64_Half   e_phnum;  /* Number of program header entries */
  Elf64_Half   e_shentsize;  /* Size of section header entry */
  Elf64_Half   e_shnum;  /* Number of section header entries */
  Elf64_Half   e_shstrndx; /* Section name string table index */
} Elf64_Ehdr;

typedef struct {
  unsigned char e_ident[EI_NIDENT]; /*16/ ELF "magic number" */
  Elf32_Half  e_type;   /* 2/ Identifies object file type */
  Elf32_Half  e_machine;  /* 2/ Specifies required architecture */
  Elf32_Word  e_version;  /* 4/ Identifies object file version */
  Elf32_Addr  e_entry;  /* 4/ Entry point virtual address */
  Elf32_Off   e_phoff;  /* 4/ Program header table file offset */
  Elf32_Off   e_shoff;  /* 4/ Section header table file offset */
  Elf32_Word  e_flags;  /* 4/ Processor-specific flags */
  Elf32_Half  e_ehsize; /* 2/ ELF header size in bytes */
  Elf32_Half  e_phentsize;  /* 2/ Program header table entry size */
  Elf32_Half    e_phnum;  /* 2/ Program header table entry count */
  Elf32_Half  e_shentsize;  /* 2/ Section header table entry size */
  Elf32_Half  e_shnum;  /* 2/ Section header table entry count */
  Elf32_Half  e_shstrndx; /* 2/ Section header string table index */
} Elf32_Ehdr;


/*============================================================================
   The ELFCLASS* macros are the defined values of e_ident[EI_CLASS].
============================================================================*/

#define ELFCLASSNONE  0 /* Invalid class */
#define ELFCLASS32  1 /* 32-bit objects */
#define ELFCLASS64  2 /* 64-bit objects */

#define ELFCLASS64_A  200 /* 64-bit objects (interim definition) */


/*============================================================================
   The ELFDATA* macros are the allowed values of e_ident[EI_DATA].
============================================================================*/

#define ELFDATANONE 0 /* Invalid data encoding */
#define ELFDATA2LSB 1 /* 2's complement, LSB at lowest byte address */
#define ELFDATA2MSB 2 /* 2's complement, MSB at lowest byte address */


/*============================================================================
   The ELFOSIABI* macros are the allowed values of e_ident[EI_OSABI].
============================================================================*/

#define ELFOSABI_SYSV   0 /* System V ABI, third edition, no checking */
#define ELFOSABI_HPUX   1 /* HP-UX operating system */

#define ELFOSABI_STANDALONE 255 /* STANDALONE operating system */


/*============================================================================
   The ELFABIVERSION* macros are the allowed values of e_ident[EI_ABIVERSION].
============================================================================*/

#define ELFABI_HPUX_NONE  0 /* None specified */
#define ELFABI_HPUX_REL11 1


/*============================================================================
   The ET_* macros define the values of the e_type field of the ElfXX_Ehdr
   structure.
============================================================================*/

#define ET_NONE   0 /* No file type */
#define ET_REL    1 /* Relocatable file */
#define ET_EXEC   2 /* Executable file */
#define ET_DYN    3 /* Shared object file */
#define ET_CORE   4 /* Core file */

#define ET_LOOS   0xfe00 /* OS-specific lowest value */
#define ET_HIOS   0xfeff /* OS-specific highest value */

#define ET_LOPROC 0xff00 /* Processor-specific lowest value */
#define ET_HIPROC 0xffff /* Processor-specific highest value */


/*============================================================================
   The EV_* macros are the allowed values of the e_version field of
   ElfXX_Ehdr;
============================================================================*/

#define EV_NONE   0 /* Invalid version */
#define EV_CURRENT  1 /* Current version */


/*============================================================================
   The ELFMAG* macros are the values of e_ident[EI_MAG0-4]
============================================================================*/

#define ELFMAG0     0x7f      /* magic number, byte 0 */
#define ELFMAG1     'E'       /* magic number, byte 1 */
#define ELFMAG2     'L'       /* magic number, byte 2 */
#define ELFMAG3     'F'       /* magic number, byte 3 */
#define ELFMAG      "\177ELF" /* magic string */
#define SELFMAG     4         /* magic string length */


/*============================================================================
   ElfXX_Phdr - program header structure.  Element of the array of
   program headers.
============================================================================*/

typedef struct {
  Elf64_Word   p_type;   /* Type of segment */
  Elf64_Word   p_flags;  /* Segment attributes */
  Elf64_Off    p_offset; /* Offset in file */
  Elf64_Addr   p_vaddr;  /* Virtual address in memory */
  Elf64_Addr   p_paddr;  /* Reserved */
  Elf64_Xword    p_filesz; /* Size of segment in file */
  Elf64_Xword    p_memsz;  /* Size of segment in memory */
  Elf64_Xword    p_align;  /* Alignment of segment */
} Elf64_Phdr;

typedef struct {
  Elf32_Word   p_type;   /* Type of segment */
  Elf32_Off    p_offset; /* Offset in file */
  Elf32_Addr   p_vaddr;  /* Virtual address in memory */
  Elf32_Addr   p_paddr;  /* Reserved */
  Elf32_Xword    p_filesz; /* Size of segment in file */
  Elf32_Xword    p_memsz;  /* Size of segment in memory */
  Elf32_Word   p_flags;  /* Segment attributes */
  Elf32_Xword    p_align;  /* Alignment of segment */
} Elf32_Phdr;


/*============================================================================
   The PF_* macros are the segment flag bits in p_flags of ElfXX_Phdr.
============================================================================*/

#define PF_X    0x1 /* Execute */
#define PF_W    0x2 /* Write */
#define PF_R    0x4 /* Read */

#define PF_MASKOS 0x0ff00000 /* OS-specific */

#define PF_MASKPROC 0xf0000000 /* Processor-specific */


/*============================================================================
   The PT_* macros are the values of p_type in ElfXX_Phdr.
============================================================================*/

#define PT_NULL   0 /* Array element is unused.  Ignore. */
#define PT_LOAD   1 /* Loadable segment. */
#define PT_DYNAMIC  2 /* Dynamic Section */
#define PT_INTERP 3 /* Section is a path to an interpreter */
#define PT_NOTE   4 /* Note Section */
#define PT_SHLIB  5 /* Currently unspecified semantics */
#define PT_PHDR   6 /* Program header table. */

#define PT_LOOS   0x60000000  /* Lowest OS-specific type */
#define PT_HIOS   0x6fffffff  /* Highest OS-specific type */

#define PT_LOPROC 0x70000000  /* Lowest Processor-specific type */
#define PT_HIPROC 0x7fffffff  /* Highest Processor-specific type */


/*============================================================================
   ElfXX_Shdr - structure for an ELF section header.  Element of the array
   of section headers.
============================================================================*/

typedef struct {
  Elf64_Word   sh_name;  /* Section name */
  Elf64_Word   sh_type;  /* Section type */
  Elf64_Xword    sh_flags; /* Section attributes */
  Elf64_Addr   sh_addr;  /* Virtual address in memory */
  Elf64_Off    sh_offset;  /* Offset in file */
  Elf64_Xword    sh_size;  /* Size of section */
  Elf64_Word   sh_link;  /* Link to other section */
  Elf64_Word   sh_info;  /* Miscellaneous information */
  Elf64_Xword    sh_addralign; /* Address alignment boundary */
  Elf64_Xword    sh_entsize; /* Entry size, if section has table */
} Elf64_Shdr;

typedef struct {
  Elf32_Word   sh_name;  /* Section name */
  Elf32_Word   sh_type;  /* Section type */
  Elf32_Xword    sh_flags; /* Section attributes */
  Elf32_Addr   sh_addr;  /* Virtual address in memory */
  Elf32_Off    sh_offset;  /* Offset in file */
  Elf32_Xword    sh_size;  /* Size of section */
  Elf32_Word   sh_link;  /* Link to other section */
  Elf32_Word   sh_info;  /* Miscellaneous information */
  Elf32_Xword    sh_addralign; /* Address alignment boundary*/
  Elf32_Xword    sh_entsize; /* Entry size, if section has table */
} Elf32_Shdr;


/*============================================================================
   The ELF_STRING_xxx macros are names of common sections
============================================================================*/

#define ELF_STRING_bss    ".bss"
#define ELF_STRING_hbss   ".hbss"
#define ELF_STRING_sbss   ".sbss"
#define ELF_STRING_tbss   ".tbss"
#define ELF_STRING_data   ".data"
#define ELF_STRING_hdata  ".hdata"
#define ELF_STRING_sdata  ".sdata"
#define ELF_STRING_fini   ".fini"
#define ELF_STRING_init   ".init"
#define ELF_STRING_interp ".interp"
#define ELF_STRING_rodata ".rodata"
#define ELF_STRING_text   ".text"
#define ELF_STRING_comment  ".comment"
#define ELF_STRING_dynamic  ".dynamic"
#define ELF_STRING_dynstr ".dynstr"
#define ELF_STRING_dynsym ".dynsym"
#define ELF_STRING_dlt    ".dlt"
#define ELF_STRING_note   ".note"
#define ELF_STRING_opd          ".opd"
#define ELF_STRING_plt    ".plt"
#define ELF_STRING_bss_rela     ".rela.bss"
#define ELF_STRING_hbss_rela  ".rela.hbss"
#define ELF_STRING_data_rela    ".rela.data"
#define ELF_STRING_dlt_rela     ".rela.dlt"
#define ELF_STRING_plt_rela     ".rela.plt"
#define ELF_STRING_sdata_rela   ".rela.sdata"
#define ELF_STRING_strtab ".strtab"
#define ELF_STRING_symtab ".symtab"
#define ELF_STRING_hash   ".hash"
#define ELF_STRING_shstrtab ".shstrtab"
#define ELF_STRING_shsymtab ".shsymtab"
#define ELF_STRING_rela   ".rela"
#define ELF_STRING_rel    ".rel"


/*============================================================================
   The SHF_* macros are the allowed values of the sh_flags field of
   ElfXX_Shdr.  These 1-bit flags define attributes of a section.
============================================================================*/

#define SHF_WRITE 0x1 /* Writable during execution */
#define SHF_ALLOC 0x2 /* Occupies memory during execution */
#define SHF_EXECINSTR 0x4 /* Contains executable instructions */

#define SHF_MASKPROC  0xf0000000 /* mask for processor-specific bits */


/*============================================================================
   SHN_* macros are reserved section header indices.  An object file will
   not have sections for these special indices.
============================================================================*/

#define SHN_UNDEF 0 /* undefined, e.g. undefined symbol */

#define SHN_LORESERVE 0xff00 /* Lower bound of reserved indices */

#define SHN_LOPROC  0xff00 /* Lower bound processor-specific index */
#define SHN_HIPROC  0xff1f/* Upper bound processor-specific index */

#define SHN_LOOS  0xff20 /* Lower bound OS-specific index */
#define SHN_HIOS  0xff3f /* Upper bound OS-specific index */

#define SHN_ABS   0xfff1 /* Absolute value, not relocated */
#define SHN_COMMON  0xfff2/* FORTRAN common or unallocated C */

#define SHN_HIRESERVE 0xffff /* Upper bound of reserved indices */


/*============================================================================
   SHT_* macros are the values of sh_type in ElfXX_Shdr
============================================================================*/

#define SHT_NULL  0 /* Inactive section header */
#define SHT_PROGBITS  1 /* Information defined by the program */
#define SHT_SYMTAB  2 /* Symbol table - not DLL */
#define SHT_STRTAB  3 /* String table */
#define SHT_RELA  4 /* Explicit addend relocations, Elf64_Rela */
#define SHT_HASH  5 /* Symbol hash table */
#define SHT_DYNAMIC 6 /* Information for dynamic linking */
#define SHT_NOTE  7 /* A Note section */
#define SHT_NOBITS  8 /* Like SHT_PROGBITS with no data */
#define SHT_REL   9 /* Implicit addend relocations, Elf64_Rel */
#define SHT_SHLIB 10 /* Currently unspecified semantics */
#define SHT_DYNSYM  11 /* Symbol table for a DLL */

#define SHT_LOOS  0x60000000 /* Lowest OS-specific section type */
#define SHT_HIOS  0x6fffffff /* Highest OS-specific section type */

#define SHT_LOPROC  0x70000000 /* Lowest processor-specific section type */
#define SHT_HIPROC  0x7fffffff /* Highest processor-specific section type */


/*============================================================================
   ElfXX_Sym - ELF symbol structure.
============================================================================*/

typedef struct {
  Elf64_Word   st_name;  /* Symbol name */
  unsigned char  st_info;  /* Type and Binding attributes */
  unsigned char  st_other; /* Reserved */
  Elf64_Half   st_shndx; /* Section table index */
  Elf64_Addr   st_value; /* Symbol value */
  Elf64_Xword    st_size;  /* Size of object (e.g., common) */
} Elf64_Sym;

typedef struct {
  Elf32_Word   st_name;  /* Symbol name */
  Elf32_Addr   st_value; /* Symbol value */
  Elf32_Xword    st_size;  /* Size of object (e.g., common) */
  unsigned char  st_info;  /* Type and Binding attributes */
  unsigned char  st_other; /* Reserved */
  Elf32_Half   st_shndx; /* Section table index */
} Elf32_Sym;


/*============================================================================
   The ELFXX_ST_* macros operate on the st_info field of ElfXX_Sym
   to extract binding attributes, type information and how to build
   an st_info value from binding and type information.
============================================================================*/

#define ELF64_ST_BIND(val)    ((val) >> 4)
#define ELF64_ST_TYPE(val)    ((val) & 0xf)
#define ELF64_ST_INFO(bind,type)  (((bind) << 4) + ((type) & 0xf))

#define ELF32_ST_BIND(val)    ((val) >> 4)
#define ELF32_ST_TYPE(val)    ((val) & 0xf)
#define ELF32_ST_INFO(bind,type)  (((bind) << 4) + ((type) & 0xf))


/*============================================================================
   STN_UNDEF (zero) designeates the first entry in the symbol table and serves
   as the undefined symbol index.
============================================================================*/

#define STN_UNDEF 0


/*============================================================================
   The STB_* macros are the defined values of the binding information part
   of st_info in ElfXX_Sym.
============================================================================*/

#define STB_LOCAL 0 /* Local symbols */
#define STB_GLOBAL  1 /* Globally visible symbols */
#define STB_WEAK  2 /* Global symbols with lower precedence */

#define STB_LOOS  10  /* Lowest OS-specific value */
#define STB_HIOS  12  /* Highest OS-specific value */

#define STB_LOPROC  13  /* Lowest processor-specific value */
#define STB_HIPROC  15  /* Highest processor-specific value */


/*===========================================================================
   The STT_* macros are the allowed values of the type information part of
   st_info in ElfXX_Sym.
============================================================================*/

#define STT_NOTYPE  0 /* Undefined symbol type */
#define STT_OBJECT  1 /* A data object, variable, array, etc. */
#define STT_FUNC  2 /* A function or other executable code */
#define STT_SECTION 3 /* Symbol associated with a section */
#define STT_FILE  4 /* Typically the name of the source file */

#define STT_LOOS  10  /* Lowest OS-specific symbol type */
#define STT_HIOS  12  /* Highest OS-specific symbol type */

#define STT_LOPROC  13  /* Lowest processor-specific symbol type */
#define STT_HIPROC  15  /* Highest processor-specific symbol type */


/*============================================================================
   ElfXX_Rel - ELF relocation without an explicit addend.
============================================================================*/

typedef struct {
  Elf64_Addr   r_offset; /* Address of reference */
  Elf64_Xword    r_info;   /* Symbol index, relocation type */
} Elf64_Rel;

typedef struct {
  Elf32_Addr   r_offset; /* Address of reference */
  Elf32_Xword    r_info;   /* Symbol index, reloc. type */
} Elf32_Rel;

/*============================================================================
   ElfXX_Rela - ELF relocation with an explicit addend.
============================================================================*/

typedef struct {
  Elf64_Addr   r_offset; /* Address of reference */
  Elf64_Xword    r_info;   /* Symbol index, relocation type */
  Elf64_Sxword   r_addend; /* Constant part of expression */
} Elf64_Rela;

typedef struct {
  Elf32_Addr   r_offset; /* Address of reference */
  Elf32_Xword    r_info;         /* Symbol index, reloc. type */
  Elf32_Sword    r_addend; /* Constant part of expression */
} Elf32_Rela;


/*============================================================================
   The ELFXX_R_* macros show how to extract the type and index parts
   of r_info in ElfXX_Rel and ElfXX_Rela and how to build r_info
   from a symbol index and type.
============================================================================*/

#define ELF64_R_INFO(sym,type)  \
  ((((Elf64_Addr) (sym)) << 32) + ((Elf64_Addr) (type)))
#define ELF64_R_SYM(info)       \
  ((Elf64_Sword) (((Elf64_Addr) (info)) >> 32))
#define ELF64_R_TYPE(info)      \
  ((Elf64_Word) (((Elf64_Addr) (info)) & 0xffffffffLL))

#define ELF32_R_INFO(sym,type)  (((sym) << 8) + ((unsigned char) (type)))
#define ELF32_R_SYM(info) ((info) >> 8)
#define ELF32_R_TYPE(info)  ((unsigned char) (info))


/*============================================================================
   ElfXX_Dyn - dynamic array entry.
============================================================================*/

typedef struct {
  Elf32_Sword    d_tag;
  union {
    Elf32_Word  d_val;  /* unsigned word */
    Elf32_Addr  d_ptr;  /* address */
  }  d_un;
} Elf32_Dyn;

extern Elf32_Dyn  _DYNAMIC32[];

typedef struct {
  Elf64_Sxword   d_tag;
  union {
    Elf64_Xword d_val;  /* unsigned word */
    Elf64_Addr  d_ptr;  /* address */
  }  d_un;
} Elf64_Dyn;

extern Elf64_Dyn        _DYNAMIC[];


/*============================================================================
   The DT_* defines are the allowed values of d_tag in ElfXX_dyn.
   These are the Dynamic Array types.
============================================================================*/

/* (i)gnore (m)andatory (o)ptional */
/* d_un   Exec  DLL */
/* ----   ----  --- */
#define DT_NULL   0   /* ignored  m m   */
#define DT_NEEDED 1   /* d_val  o o   */
#define DT_PLTRELSZ 2   /* d_val  o o   */
#define DT_PLTGOT 3   /* d_ptr  o o   */
#define DT_HASH   4   /* d_ptr  m m   */
#define DT_STRTAB 5   /* d_ptr  m m   */
#define DT_SYMTAB 6   /* d_ptr  m m   */
#define DT_RELA   7   /* d_ptr  m o   */
#define DT_RELASZ 8   /* d_val  m o   */
#define DT_RELAENT  9   /* d_val  m o   */
#define DT_STRSZ  10    /* d_val  m   m   */
#define DT_SYMENT 11    /* d_val  m m   */
#define DT_INIT   12    /* d_ptr  o o   */
#define DT_FINI   13    /* d_ptr  o o   */
#define DT_SONAME 14    /* d_val  i o   */
#define DT_RPATH  15    /* d_val  o i   */
#define DT_SYMBOLIC 16    /* ignored  i o   */
#define DT_REL    17    /* d_ptr  m o   */
#define DT_RELSZ  18    /* d_val  m o   */
#define DT_RELENT 19    /* d_val  m o   */
#define DT_PLTREL 20    /* d_val  o o   */
#define DT_DEBUG  21    /* d_ptr  o i   */
#define DT_TEXTREL  22    /* ignored  o o   */
#define DT_JMPREL 23    /* d_ptr  o o   */
#define DT_BIND_NOW 24    /* ignored */
#define DT_INIT_ARRAY   25    /* d_ptr  o o   */
#define DT_FINI_ARRAY 26    /* d_ptr  o o   */
#define DT_INIT_ARRAYSZ 27    /* d_val  o o   */
#define DT_FINI_ARRAYSZ 28    /* d_val        o       o   */

#define DT_LOOS   0x60000000  /* unspecified - OS specific */
#define DT_HIOS   0x6fffffff  /* unspecified - OS specific */

#define DT_LOPROC 0x70000000  /* unspecified - processor specific */
#define DT_HIPROC 0x7fffffff  /* unspecified - processor specific */


#ifndef _HP_NO_ELF_DEPRICATED

/* These defines have been depricated and will be removed in a future release.*/

#define ELFOSABI_NONE   0 /* None specified */
#define ELFOSABI_NT   2 /* NT operating system */
#define ELFOSABI_SCOUNIX  3 /* SCO UNIX operating system */
#define SHF_COMDAT  0x8 /* Is a member of a COMDAT group */
#define SHT_COMDAT  12 /* COMDAT group directory -> SHT_HP_COMDAT */

#endif /* ifndef _HP_NO_ELF_DEPRICATED */

#ifdef __cplusplus
}
#endif /* ifndef _HP_NO_ELF_DEPRICATED */

#if defined(ELF_TARGET_ALL) || defined(ELF_TARGET_PARISC)
#include <elf_parisc.h>
#endif

#endif /* ELF_INCLUDED */
