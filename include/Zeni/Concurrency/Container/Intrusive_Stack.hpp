#include "../Internal/Concurrency.hpp"

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
#include "None/Intrusive_Stack.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
#include "Locking/Intrusive_Stack.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#include "Lockfree/Intrusive_Stack.hpp"
#endif
