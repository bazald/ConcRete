#include "Zeni/Rete/Node.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  std::shared_ptr<const Node> Node::shared_from_this() const {
    return std::static_pointer_cast<const Node>(Concurrency::Recipient::shared_from_this());
  }

  std::shared_ptr<Node> Node::shared_from_this() {
    return std::static_pointer_cast<Node>(Concurrency::Recipient::shared_from_this());
  }

  Node::Node(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash)
    : m_height(height), m_size(size), m_token_size(token_size),
    m_hash(hash)
  {
  }

  int64_t Node::get_height() const {
    return m_height;
  }

  int64_t Node::get_size() const {
    return m_size;
  }

  int64_t Node::get_token_size() const {
    return m_token_size;
  }

  void Node::receive(const Message_Connect_Filter_0 &)
  {
    abort();
  }

  void Node::receive(const Message_Connect_Filter_1 &)
  {
    abort();
  }

  void Node::receive(const Message_Connect_Filter_2 &)
  {
    abort();
  }

  void Node::receive(const Message_Connect_Join &)
  {
    abort();
  }

  void Node::receive(const std::shared_ptr<const Concurrency::Message> message) noexcept {
    std::dynamic_pointer_cast<const Rete::Message>(message)->receive();
  }

  size_t Node::get_hash() const {
    return m_hash;
  }

}
