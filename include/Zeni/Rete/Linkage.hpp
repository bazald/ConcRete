#ifndef ZENI_RETE_LINKAGE_H
#define ZENI_RETE_LINKAGE_H

#if !defined(_WINDOWS)
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE __attribute__ ((visibility ("default")))
#elif defined(RETE_EXPORTS)
#define ZENI_RETE_EXTERN
#define ZENI_RETE_LINKAGE __declspec(dllexport)
#else
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE __declspec(dllimport)
#endif

#endif
