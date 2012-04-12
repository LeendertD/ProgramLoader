#ifndef TYPES_H
#define TYPES_H

/***
 *  Leendert van Duijn:
 *  Copied this file from the microgrid framework, moddified for C (no ++)
 *  usage. Some functions are needed, others could be removed. Most importantly
 *  is the usage as a local header, nothing should be exported from this.
 *
 ***/


/*Leendert: Removed as this is just included for some types.
// Include the configuration defines
#include "sys_config.h"
*/
// Define the GCC version for checking for certain extensions
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION > 20300
// The printf-format attribute is available since GCC 2.3
# define FORMAT_PRINTF(fmt,va) __attribute__ ((format (printf, fmt, va)))
#else
# define FORMAT_PRINTF(fmt,va)
#endif

// Support non-compliant C99 compilers (different long long type)
#if defined(_MSC_VER)
typedef __int8           int8_t;
typedef __int16          int16_t;
typedef __int32          int32_t;
typedef __int64          int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

static const int8_t   INT8_MIN  = -128;
static const int8_t   INT8_MAX  =  127;
static const int16_t  INT16_MIN = -32768;
static const int16_t  INT16_MAX =  32767;
static const int32_t  INT32_MIN = 0x80000000L;
static const int32_t  INT32_MAX = 0x7FFFFFFFL;
static const int64_t  INT64_MIN = 0x8000000000000000LL;
static const int64_t  INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const uint8_t  UINT8_MIN  = 0U;
static const uint8_t  UINT8_MAX  = 0xFFU;
static const uint16_t UINT16_MIN = 0U;
static const uint16_t UINT16_MAX = 0xFFFFU;
static const uint32_t UINT32_MIN = 0UL;
static const uint32_t UINT32_MAX = 0xFFFFFFFFUL;
static const uint64_t UINT64_MIN = 0ULL;
static const uint64_t UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
#else
// We just hope that this compiler properly supports the C++ standard
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#endif

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

// Define endianness conversion routines
static inline uint16_t letohs(uint16_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static inline uint32_t letohl(uint32_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static inline uint64_t letohll(uint64_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return ((uint64_t)b[0] <<  0) | ((uint64_t)b[1] <<  8) | ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24) |
           ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40) | ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
}

static inline uint16_t betohs(uint16_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return (uint16_t)((uint16_t)b[1] | ((uint16_t)b[0] << 8));
}

static inline uint32_t betohl(uint32_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return (uint32_t)b[3] | ((uint32_t)b[2] << 8) | ((uint32_t)b[1] << 16) | ((uint32_t)b[0] << 24);
}

static inline uint64_t betohll(uint64_t x) {
    const uint8_t* b = (const uint8_t*)(&x);
    return ((uint64_t)b[7] <<  0) | ((uint64_t)b[6] <<  8) | ((uint64_t)b[5] << 16) | ((uint64_t)b[4] << 24) |
           ((uint64_t)b[3] << 32) | ((uint64_t)b[2] << 40) | ((uint64_t)b[1] << 48) | ((uint64_t)b[0] << 56);
}

#endif



#ifndef ELF_H
#define ELF_H


/*Leendert: Edited for moving into stand alone loader*/
//#include "types.h"

// Some ELF types
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sxword;
typedef uint32_t Elf32_Xword;

typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off;
typedef int32_t  Elf64_Sword;
typedef int64_t  Elf64_Sxword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;

#define EI_NIDENT_D 16

static const int EI_NIDENT  = EI_NIDENT_D;
static const int EI_MAG0    = 0; // File identification
static const int EI_MAG1    = 1; // File identification
static const int EI_MAG2    = 2; // File identification
static const int EI_MAG3    = 3; // File identification
static const int EI_CLASS   = 4; // File class
static const int EI_DATA    = 5; // Data encoding
static const int EI_VERSION = 6; // File version
static const int EI_PAD     = 7; // Start of padding bytes

static const unsigned char ELFMAG0 = 0x7F; // e_ident[EI_MAG0]
static const unsigned char ELFMAG1 = 'E';  // e_ident[EI_MAG1]
static const unsigned char ELFMAG2 = 'L';  // e_ident[EI_MAG2]
static const unsigned char ELFMAG3 = 'F';  // e_ident[EI_MAG3]

// e_ident[EI_CLASS]
#define ELFCLASSNONE 0 // Invalid class
#define ELFCLASS32   1 // 32-bit objects
#define ELFCLASS64   2 // 64-bit objects

// e_ident[EI_DATA]
#define ELFDATANONE 0 // Invalid data encoding
#define ELFDATA2LSB 1 // 2's complement values, with the LSB occupying the lowest address.
#define ELFDATA2MSB 2 // 2's complement values, with the MSB occupying the lowest address.

// e_ident[EI_VERSION]
static const unsigned char EV_NONE    = 0; // Invalid version
static const unsigned char EV_CURRENT = 1; // Current version

//#if defined(TARGET_MTALPHA)
#define ELFCLASS ELFCLASS64
#define ELFDATA  ELFDATA2LSB

//#elif defined(TARGET_MTSPARC)
//#define ELFCLASS ELFCLASS32
//#define ELFDATA  ELFDATA2MSB
//#endif

#if ELFDATA == ELFDATA2MSB
static inline uint64_t elftohll(uint64_t x) { return betohll(x); }
static inline uint32_t elftohl(uint32_t x)  { return betohl(x); }
static inline uint16_t elftohs(uint16_t x)  { return betohs(x); }
#else
static inline uint64_t elftohll(uint64_t x) { return letohll(x); }
static inline uint32_t elftohl(uint32_t x)  { return letohl(x); }
static inline uint16_t elftohs(uint16_t x)  { return letohs(x); }
#endif

#if ELFCLASS == ELFCLASS64
typedef Elf64_Addr   Elf_Addr;
typedef Elf64_Half   Elf_Half;
typedef Elf64_Off    Elf_Off;
typedef Elf64_Sword  Elf_Sword;
typedef Elf64_Word   Elf_Word;
typedef Elf64_Sxword Elf_Sxword;
typedef Elf64_Xword  Elf_Xword;

static inline Elf_Addr   elftoha  (Elf_Addr   x) { return elftohll(x); }
static inline Elf_Half   elftohh  (Elf_Half   x) { return elftohs(x); }
static inline Elf_Off    elftoho  (Elf_Off    x) { return elftohll(x); }
static inline Elf_Sword  elftohsw (Elf_Sword  x) { return elftohl(x); }
static inline Elf_Word   elftohw  (Elf_Word   x) { return elftohl(x); }
static inline Elf_Sxword elftohsxw(Elf_Sxword x) { return elftohll(x); }
static inline Elf_Xword  elftohxw (Elf_Xword  x) { return elftohll(x); }

#else
typedef Elf32_Addr   Elf_Addr;
typedef Elf32_Half   Elf_Half;
typedef Elf32_Off    Elf_Off;
typedef Elf32_Sword  Elf_Sword;
typedef Elf32_Word   Elf_Word;
typedef Elf32_Sxword Elf_Sxword;
typedef Elf32_Xword  Elf_Xword;

static inline Elf_Addr   elftoha  (Elf_Addr   x) { return elftohl(x); }
static inline Elf_Half   elftohh  (Elf_Half   x) { return elftohs(x); }
static inline Elf_Off    elftoho  (Elf_Off    x) { return elftohl(x); }
static inline Elf_Sword  elftohsw (Elf_Sword  x) { return elftohl(x); }
static inline Elf_Word   elftohw  (Elf_Word   x) { return elftohl(x); }
static inline Elf_Sxword elftohsxw(Elf_Sxword x) { return elftohl(x); }
static inline Elf_Xword  elftohxw (Elf_Xword  x) { return elftohl(x); }
#endif

static const Elf_Half ET_NONE   = 0; // No file type
static const Elf_Half ET_REL    = 1; // Relocatable file
static const Elf_Half ET_EXEC   = 2; // Executable file
static const Elf_Half ET_DYN    = 3; // Shared object file
static const Elf_Half ET_CORE   = 4; // Core file
static const Elf_Half ET_LOPROC = 0xff00; // Processor-specific
static const Elf_Half ET_HIPROC = 0xffff; // Processor-specific

// Legal values for e_machine (architecture)
static const Elf_Half EM_NONE        =  0; // No machine
static const Elf_Half EM_M32         =  1; // AT&T WE 32100
static const Elf_Half EM_SPARC       =  2; // SPARC
static const Elf_Half EM_386         =  3; // Intel 80386
static const Elf_Half EM_68K         =  4; // Motorola 68000
static const Elf_Half EM_88K         =  5; // Motorola 88000
static const Elf_Half EM_860         =  7; // Intel 80860
static const Elf_Half EM_MIPS        =  8; // MIPS R3000 big-endian
static const Elf_Half EM_S370        =  9; // Amdahl
static const Elf_Half EM_MIPS_RS4_BE = 10; // MIPS R4000 big-endian
static const Elf_Half EM_RS6000      = 11; // RS6000

static const Elf_Half EM_PARISC      = 15; // HPPA
static const Elf_Half EM_nCUBE       = 16; // nCUBE
static const Elf_Half EM_VPP500      = 17; // Fujitsu VPP500
static const Elf_Half EM_SPARC32PLUS = 18; // Sun's "v8plus"
static const Elf_Half EM_960         = 19; // Intel 80960
static const Elf_Half EM_PPC         = 20; // PowerPC

static const Elf_Half EM_V800        = 36; // NEC V800 series
static const Elf_Half EM_FR20        = 37; // Fujitsu FR20
static const Elf_Half EM_RH32        = 38; // TRW RH32
static const Elf_Half EM_MMA         = 39; // Fujitsu MMA
static const Elf_Half EM_ARM         = 40; // ARM
static const Elf_Half EM_FAKE_ALPHA  = 41; // Digital Alpha
static const Elf_Half EM_SH          = 42; // Hitachi SH
static const Elf_Half EM_SPARCV9     = 43; // SPARC v9 64-bit
static const Elf_Half EM_TRICORE     = 44; // Siemens Tricore
static const Elf_Half EM_ARC         = 45; // Argonaut RISC Core
static const Elf_Half EM_H8_300      = 46; // Hitachi H8/300
static const Elf_Half EM_H8_300H     = 47; // Hitachi H8/300H
static const Elf_Half EM_H8S         = 48; // Hitachi H8S
static const Elf_Half EM_H8_500      = 49; // Hitachi H8/500
static const Elf_Half EM_IA_64       = 50; // Intel Merced
static const Elf_Half EM_MIPS_X      = 51; // Stanford MIPS-X
static const Elf_Half EM_COLDFIRE    = 52; // Motorola Coldfire
static const Elf_Half EM_68HC12      = 53; // Motorola M68HC12

// unofficial EM_* values
static const Elf_Half EM_ALPHA       = 0x9026; // Alpha
static const Elf_Half EM_MTALPHA     = 0xafef; // Microthreaded Alpha
static const Elf_Half EM_MTSPARC     = 0xaff0; // Microthreaded Sparc V8

//#if defined(TARGET_MTALPHA)
#define MACHINE_NORMAL EM_MTALPHA
#define MACHINE_LEGACY EM_ALPHA
//#elif defined(TARGET_MTSPARC)
//#define MACHINE_NORMAL EM_MTSPARC
//#define MACHINE_LEGACY EM_SPARC
//#endif

// File header
#pragma pack(1)
struct Elf_Ehdr
{
	unsigned char e_ident[EI_NIDENT_D];
	Elf_Half	  e_type;
	Elf_Half	  e_machine;
	Elf_Word	  e_version;
	Elf_Addr	  e_entry;
	Elf_Off	      e_phoff;
	Elf_Off	      e_shoff;
	Elf_Word	  e_flags;
	Elf_Half	  e_ehsize;
	Elf_Half	  e_phentsize;
	Elf_Half	  e_phnum;
	Elf_Half	  e_shentsize;
	Elf_Half	  e_shnum;
	Elf_Half	  e_shstrndx;
};
#pragma pack()

static const Elf_Word PT_NULL    = 0;
static const Elf_Word PT_LOAD    = 1;
static const Elf_Word PT_DYNAMIC = 2;
static const Elf_Word PT_INTERP  = 3;
static const Elf_Word PT_NOTE    = 4;
static const Elf_Word PT_SHLIB   = 5;
static const Elf_Word PT_PHDR    = 6;
static const Elf_Word PT_LOPROC  = 0x70000000;
static const Elf_Word PT_HIPROC  = 0x7fffffff;

static const Elf_Word PF_R = 4;
static const Elf_Word PF_W = 2;
static const Elf_Word PF_X = 1;

// Program header
#pragma pack(1)
#if ELFCLASS == ELFCLASS64
struct Elf_Phdr
{
	Elf_Word  p_type;
	Elf_Word  p_flags;
	Elf_Off   p_offset;
	Elf_Addr  p_vaddr;
	Elf_Addr  p_paddr;
	Elf_Xword p_filesz;
	Elf_Xword p_memsz;
	Elf_Xword p_align;
};
#else
struct Elf_Phdr
{
	Elf_Word  p_type;
	Elf_Off   p_offset;
	Elf_Addr  p_vaddr;
	Elf_Addr  p_paddr;
	Elf_Xword p_filesz;
	Elf_Xword p_memsz;
	Elf_Word  p_flags;
	Elf_Xword p_align;
};
#endif
#pragma pack()

#endif

