#ifndef ZENI_CONCURRENCY_LINKAGE_HPP
#define ZENI_CONCURRENCY_LINKAGE_HPP

#if !defined(_WIN32)
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE __attribute__ ((visibility ("default")))
#else
#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#if defined(CONCURRENCY_EXPORTS)
#define ZENI_CONCURRENCY_EXTERN
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllexport)
#else
#define ZENI_CONCURRENCY_EXTERN extern
#define ZENI_CONCURRENCY_LINKAGE __declspec(dllimport)
#endif
#endif

#endif
