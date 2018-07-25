#include "../Internal/Concurrency.hpp"

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
#include "None/Intrusive_Shared_Ptr.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
#include "Locking/Intrusive_Shared_Ptr.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#include "Lockfree/Intrusive_Shared_Ptr.hpp"
#endif
