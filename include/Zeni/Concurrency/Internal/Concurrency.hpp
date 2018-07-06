#ifndef ZENI_CONCURRENCY_HPP
#define ZENI_CONCURRENCY_HPP

#ifndef ZENI_CONCURRENCY
#define ZENI_CONCURRENCY ZENI_CONCURRENCY_LOCKFREE
#endif

#define ZENI_CONCURRENCY_NONE 0
#define ZENI_CONCURRENCY_LOCKING 1
#define ZENI_CONCURRENCY_LOCKFREE 2

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE && ZENI_CONCURRENCY != ZENI_CONCURRENCY_LOCKING && ZENI_CONCURRENCY != ZENI_CONCURRENCY_LOCKFREE
static_assert(false, "ZENI_CONCURRENCY must be ZENI_CONCURRENCY_NONE, ZENI_CONCURRENCY_LOCKING, or ZENI_CONCURRENCY_LOCKFREE!");
#endif

#endif
