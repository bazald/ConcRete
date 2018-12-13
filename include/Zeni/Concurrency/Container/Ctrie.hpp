#include "../Internal/Concurrency.hpp"

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
#include "None/Ctrie.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
#include "Locking/Ctrie.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#include "Lockfree/Ctrie.hpp"
#endif
