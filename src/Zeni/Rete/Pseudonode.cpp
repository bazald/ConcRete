#include "Zeni/Rete/Pseudonode.hpp"

#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include <cassert>

namespace Zeni::Rete {

  void Pseudonode::receive(Concurrency::Job_Queue &, const std::shared_ptr<const Concurrency::Raven> raven) {
    std::dynamic_pointer_cast<const Rete::Raven>(raven)->receive();
  }

  void Pseudonode::receive(const Raven_Disconnect_Output &raven) {
    disconnect_output(raven.get_Network(), raven.get_sender());
  }

}
