#include "Zeni/Rete/Internal/Debug_Counters.hpp"

#ifdef ZENI_RETE_USE_DEBUG_COUNTERS

#include <iostream>

namespace Zeni::Rete::Debug_Counters {

  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_node_increments = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_try_increment_child_counts = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_connect_gates_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_connect_outputs_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_decrement_outputs_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_gates_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_output_and_decrements_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_output_but_nodecrements_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_empties_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_nonempties_received = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_tokens_inserted = 0;
  ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_tokens_removed = 0;
  ZENI_RETE_LINKAGE std::array<Zeni::Concurrency::Atomic_int64_t<true>, 8> g_extra;

  void print(std::ostream &os) {
    os << std::endl;
    os << "  g_node_increments                             = " << g_node_increments.load() << std::endl;
    os << "  g_try_increment_child_counts                  = " << g_try_increment_child_counts.load() << std::endl;
    os << "  g_connect_outputs_received                    = " << g_connect_outputs_received.load() << std::endl;
    os << "  g_connect_gates_received                      = " << g_connect_gates_received.load() << std::endl;
    os << "  g_disconnect_gates_received                   = " << g_disconnect_gates_received.load() << std::endl;
    os << std::endl;
    os << "  g_disconnect_output_and_decrements_received   = " << g_disconnect_output_and_decrements_received.load() << std::endl;
    os << "  g_disconnect_output_but_nodecrements_received = " << g_disconnect_output_but_nodecrements_received.load() << std::endl;
    os << "  g_decrement_outputs_received                  = " << g_decrement_outputs_received.load() << std::endl;
    os << std::endl;
    os << "  g_tokens_inserted                             = " << g_tokens_inserted.load() << std::endl;
    os << "  g_tokens_removed                              = " << g_tokens_removed.load() << std::endl;
    os << "  g_empties_received                            = " << g_empties_received.load() << std::endl;
    os << "  g_nonempties_received                         = " << g_nonempties_received.load() << std::endl;
    os << std::endl;
    os << "  g_extra                                       =";
    for (auto &extra : g_extra)
      os << ' ' << extra.load();
    os << std::endl;
    os << std::endl;
  }

  void reset() {
    g_node_increments.store(0);
    g_try_increment_child_counts.store(0);
    g_connect_gates_received.store(0);
    g_connect_outputs_received.store(0);
    g_decrement_outputs_received.store(0);
    g_disconnect_gates_received.store(0);
    g_empties_received.store(0);
    g_nonempties_received.store(0);
    g_disconnect_output_and_decrements_received.store(0);
    g_disconnect_output_but_nodecrements_received.store(0);
    g_tokens_inserted.store(0);
    g_tokens_removed.store(0);
    for (int i = 0; i != 8; ++i)
      g_extra[i].store(0);
  }

}

#else

namespace Zeni::Rete::Debug_Counters {

  void print(std::ostream &)
  {
  }

  void reset()
  {
  }

}

#endif
