#ifndef ZENI_CONCURRENCY_LINKAGE_H
#define ZENI_CONCURRENCY_LINKAGE_H

#if !defined(_WINDOWS)
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE
#elif defined(CONCURRENCY_EXPORTS)
#define ZENI_CONCURRENCY_EXTERN
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllexport)
#else
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllimport)
#endif

#endif
