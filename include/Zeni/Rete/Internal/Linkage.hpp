#ifndef ZENI_RETE_LINKAGE_HPP
#define ZENI_RETE_LINKAGE_HPP

#if !defined(_WIN32)
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE __attribute__ ((visibility ("default")))
#else
#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#if defined(RETE_EXPORTS)
#define ZENI_RETE_EXTERN
#define ZENI_RETE_LINKAGE __declspec(dllexport)
#else
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE __declspec(dllimport)
#endif
#endif

#endif
