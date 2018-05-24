#include "Zeni/Rete/Pseudonode.hpp"

#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    void Pseudonode::receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) {
      dynamic_cast<const Rete::Raven *>(&raven)->receive();
    }

    void Pseudonode::receive(const Raven_Disconnect_Output &raven) {
      disconnect_output(raven.get_Network(), raven.get_sender());
    }

  }

}
