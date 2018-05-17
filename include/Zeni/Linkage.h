#ifndef ZENI_LINKAGE_H
#define ZENI_LINKAGE_H

#if !defined(_WINDOWS)
#define ZENI_EXTERN extern
#define ZENI_LINKAGE
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE
#elif defined(ZENI_EXPORTS)
#define ZENI_EXTERN
#define ZENI_LINKAGE __declspec(dllexport)
#define ZENI_CONCURRENCY_EXTERN
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllexport)
#define ZENI_RETE_EXTERN
#define ZENI_RETE_LINKAGE __declspec(dllexport)
#else
#define ZENI_EXTERN extern
#define ZENI_LINKAGE __declspec(dllimport)
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllimport)
#define ZENI_RETE_EXTERN extern
#define ZENI_RETE_LINKAGE __declspec(dllimport)
#endif

#endif
