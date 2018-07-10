#include "../Internal/Concurrency.hpp"

//#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
//#include "None/Shared_Ptr.hpp"
//#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
//#include "Locking/Shared_Ptr.hpp"
//#elif ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKFREE
#include "Lockfree/Shared_Ptr.hpp"
#endif
