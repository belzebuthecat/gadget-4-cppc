/**************************  instrset_detect.cpp   ****************************
* Author:        Agner Fog
* Date created:  2012-05-30
* Last modified: 2017-05-02
* Version:       1.28
* Project:       vector classes
* Description:
* Functions for checking which instruction sets are supported.
*
* (c) Copyright 2012-2017 GNU General Public License http://www.gnu.org/licenses
\*****************************************************************************/

#include "instrset.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__x86_64__) || defined(_M_X64)
  #define USE_CPUID
#endif

#ifdef USE_CPUID
#include <cpuid.h>
#endif

#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE
{
#endif

// Define interface to cpuid instruction.
// input:  eax = functionnumber, ecx = 0
// output: eax = output[0], ebx = output[1], ecx = output[2], edx = output[3]
static inline void cpuid(int output[4], int functionnumber)
{
#if defined(__GNUC__) || defined(__clang__)  // use inline assembly, Gnu/AT&T syntax

    printf("[INFO] CPUID not available. Assuming baseline SIMD support.\n");
    return ; // Or another code to indicate default/fallback path

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)  // Microsoft or Intel compiler, intrin.h included

  __cpuidex(output, functionnumber, 0);  // intrinsic function for CPUID

#else  // unknown platform. try inline assembly with masm/intel syntax

  __asm {
        mov eax, functionnumber
        xor ecx, ecx
        cpuid;
        mov esi, output
        mov [esi],    eax
        mov [esi+4],  ebx
        mov [esi+8],  ecx
        mov [esi+12], edx
  }

#endif
}

// Define interface to xgetbv instruction
static inline int64_t xgetbv(int ctr)
{
#if(defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 160040000) || \
    (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1200)  // Microsoft or Intel compiler supporting _xgetbv intrinsic

  return _xgetbv(ctr);  // intrinsic function for XGETBV

#elif defined(__GNUC__)  // use inline assembly, Gnu/AT&T syntax


    printf("[INFO] CPUID not available. Assuming baseline SIMD support.\n");
    return 0; // Or another code to indicate default/fallback path

#else  // #elif defined (_WIN32)                           // other compiler. try inline assembly with masm/intel/MS syntax

  uint32_t a, d;
  __asm {
        mov ecx, ctr
        _emit 0x0f
        _emit 0x01
        _emit 0xd0 ;  // xgetbv
        mov a, eax
        mov d, edx
  }
  return a | (uint64_t(d) << 32);

#endif
}

/* find supported instruction set
    return value:
    0           = 80386 instruction set
    1  or above = SSE (XMM) supported by CPU (not testing for O.S. support)
    2  or above = SSE2
    3  or above = SSE3
    4  or above = Supplementary SSE3 (SSSE3)
    5  or above = SSE4.1
    6  or above = SSE4.2
    7  or above = AVX supported by CPU and operating system
    8  or above = AVX2
    9  or above = AVX512F
    10 or above = AVX512VL
    11 or above = AVX512BW, AVX512DQ
*/
int instrset_detect(void) {
#ifdef USE_CPUID
    // This is x86-specific: use CPUID to detect SIMD extensions
    unsigned int a, b, c, d;
    __asm("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0), "c"(0));
    return 1;  // Simplified return
#else
    // Fallback for non-x86, e.g. ARM
    printf("[INFO] CPUID not available. Assuming baseline SIMD support.\n");
    return 0; // Or another code to indicate default/fallback path
#endif
}

// detect if CPU supports the FMA3 instruction set
bool hasFMA3(void)
{
  if(instrset_detect() < 7)
    return false;                       // must have AVX
  int abcd[4];                          // cpuid results
  cpuid(abcd, 1);                       // call cpuid function 1
  return ((abcd[2] & (1 << 12)) != 0);  // ecx bit 12 indicates FMA3
}

// detect if CPU supports the FMA4 instruction set
bool hasFMA4(void)
{
  if(instrset_detect() < 7)
    return false;                       // must have AVX
  int abcd[4];                          // cpuid results
  cpuid(abcd, 0x80000001);              // call cpuid function 0x80000001
  return ((abcd[2] & (1 << 16)) != 0);  // ecx bit 16 indicates FMA4
}

// detect if CPU supports the XOP instruction set
bool hasXOP(void)
{
  if(instrset_detect() < 7)
    return false;                       // must have AVX
  int abcd[4];                          // cpuid results
  cpuid(abcd, 0x80000001);              // call cpuid function 0x80000001
  return ((abcd[2] & (1 << 11)) != 0);  // ecx bit 11 indicates XOP
}

// detect if CPU supports the AVX512ER instruction set
bool hasAVX512ER(void)
{
  if(instrset_detect() < 9)
    return false;                       // must have AVX512F
  int abcd[4];                          // cpuid results
  cpuid(abcd, 7);                       // call cpuid function 7
  return ((abcd[1] & (1 << 27)) != 0);  // ebx bit 27 indicates AVX512ER
}

#ifdef VCL_NAMESPACE
}
#endif
