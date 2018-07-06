#include "../Internal/Concurrency.hpp"

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
#include "None/Antiable_List.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
#include "Locking/Antiable_List.hpp"
#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#include "Lockfree/Antiable_List.hpp"
#endif
