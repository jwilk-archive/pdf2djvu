#ifndef __XML_EXPORTS_H__
#define __XML_EXPORTS_H__

#if defined(IN_LIBXML)
  #define XMLPUBFUN __declspec(dllexport)
  #define XMLPUBVAR __declspec(dllexport) extern
#else
  #define XMLPUBFUN __declspec(dllimport)
  #define XMLPUBVAR __declspec(dllimport) extern
#endif
#define XMLCALL __cdecl
#define XMLCDECL __cdecl
#if !defined _REENTRANT
  #define _REENTRANT
#endif

#endif
