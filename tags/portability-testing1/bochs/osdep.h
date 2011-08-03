//
// osdep.h
// 
// Operating system dependent includes and defines for Bochs.
// This file is included by bochs.h, which is included by almost
// everything.
//

#ifndef BX_OSDEP_H
#define BX_OSDEP_H

// This code recognizes the following preprocessor symbols for different
// operating systems:
//   macintosh
//   WIN32

#ifdef __cplusplus
extern "C" {
#endif   /* __cplusplus */



#if BX_HAVE_SNPRINTF
#define bx_snprintf snprintf
#else
  extern int bx_snprintf (char *s, size_t maxlen, const char *format, ...);
#endif


#if BX_HAVE_STRTOULL
#define bx_strtoull strtoull
#else
  extern unsigned long long bx_strtoull (const char *nptr, char **endptr, int baseignore);
#endif



#ifdef __cplusplus
}
#endif   /* __cplusplus */

#endif /* ifdef BX_OSDEP_H */