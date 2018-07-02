#ifndef ZENI_CONCURRENCY_RECLAMATION_STACKS_IMPL_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACKS_IMPL_HPP

#include "Mallocator.hpp"
#include "Reclamation_Stack.hpp"
#include "../Mutex.hpp"

#include <cstdint>
#include <list>
#include <unordered_set>

namespace Zeni::Concurrency {

  class Reclamation_Stacks_Impl {
    Reclamation_Stacks_Impl(const Reclamation_Stacks_Impl &) = delete;
    Reclamation_Stacks_Impl operator=(const Reclamation_Stacks_Impl &) = delete;

    class Clearer {
    public:
      ~Clearer() noexcept;
    };

    Reclamation_Stacks_Impl() noexcept(false);

  public:
    static Reclamation_Stacks_Impl & get() noexcept(false);

    std::shared_ptr<Reclamation_Stack> get_stack() noexcept(false);

    void clear_stack() noexcept;

    void reclaim() noexcept;

  private:
    Mutex m_mutex;
    std::unordered_set<std::shared_ptr<Reclamation_Stack>,
      std::hash<std::shared_ptr<Reclamation_Stack>>,
      std::equal_to<std::shared_ptr<Reclamation_Stack>>,
      Mallocator<std::shared_ptr<Reclamation_Stack>>> m_reclamation_stacks;
    static thread_local std::shared_ptr<Reclamation_Stack> m_reclamation_stack;
    static thread_local std::shared_ptr<Clearer> m_clearer;
  };

}

#endif
