#ifndef ZENI_RETE_CUSTOM_DATA_H
#define ZENI_RETE_CUSTOM_DATA_H

#include "Zeni/Rete/Linkage.hpp"

#include <iosfwd>
#include <memory>

namespace Zeni {

  namespace Rete {

    class Custom_Data;

  }

}

namespace Zeni {

  namespace Rete {

    class Node;

    class Custom_Data : std::enable_shared_from_this<Custom_Data> {
      Custom_Data(const Custom_Data &) = delete;
      Custom_Data & operator=(const Custom_Data &) = delete;

    public:
      ZENI_RETE_LINKAGE Custom_Data();

      ZENI_RETE_LINKAGE virtual ~Custom_Data();

      ZENI_RETE_LINKAGE virtual Custom_Data * clone() const = 0;

      ZENI_RETE_LINKAGE virtual int64_t rank() const = 0;
      ZENI_RETE_LINKAGE virtual std::shared_ptr<Node> cluster_root_ancestor() const = 0;

      ZENI_RETE_LINKAGE virtual void print_flags(std::ostream &os) const = 0;
      ZENI_RETE_LINKAGE virtual void print_action(std::ostream &os) const = 0;
      ZENI_RETE_LINKAGE virtual std::shared_ptr<const Node> get_suppress() const = 0;
    };

  }

}

#endif
