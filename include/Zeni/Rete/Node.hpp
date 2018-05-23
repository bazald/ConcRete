#ifndef ZENI_RETE_NODE_H
#define ZENI_RETE_NODE_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

//#include <algorithm>
//#include <cassert>
//#include <functional>
//#include <list>
//#include <unordered_set>
#include <deque>

namespace Zeni {

  namespace Rete {

    class Custom_Data;
    class Network;
    class Raven_Disconnect_Output;
    class Raven_Token_Insert;
    class Raven_Token_Remove;

    class Node : public Concurrency::Maester
    {
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

    public:
      typedef std::unordered_set<std::shared_ptr<Node>> Outputs;

    protected:
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this();

      ZENI_RETE_LINKAGE Node(const int64_t &height, const int64_t &size, const int64_t &token_size);

      /// Assumes the lock is held
      ZENI_RETE_LINKAGE virtual void send_disconnect_from_parents(const std::shared_ptr<Network> &network) = 0;

    public:
      ZENI_RETE_LINKAGE const Outputs & get_outputs() const;
      ZENI_RETE_LINKAGE int64_t get_height() const;
      ZENI_RETE_LINKAGE int64_t get_size() const;
      ZENI_RETE_LINKAGE int64_t get_token_size() const;

      /// Get the output count
      ZENI_RETE_LINKAGE int64_t get_output_count() const;
      /// Increment the output count
      ZENI_RETE_LINKAGE void increment_output_count();
      /// Does *not* increment the output count, under the assumption that gaining access to this object has already resulted in its increment
      ZENI_RETE_LINKAGE void connect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output);
      /// Decrements the output count, potentially resulting in cascading disconnects
      ZENI_RETE_LINKAGE void disconnect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output);

      ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) override;
      ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Output &raven);
      /// Returns true if and only if the token matches and can be inserted (no antitokens present)
      ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Insert &raven) = 0;
      /// Returns true if and only if the token matches and can be removed (at least one token present)
      ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Remove &raven) = 0;

      ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

      std::shared_ptr<Custom_Data> custom_data;

    protected:
      Outputs m_outputs;
      Tokens m_output_tokens;

    private:
      int64_t m_output_count = 1;
      int64_t m_height;
      int64_t m_size;
      int64_t m_token_size;
    };

  }

}

#endif
