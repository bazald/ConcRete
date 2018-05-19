#ifndef ZENI_CUSTOM_DATA_H
#define ZENI_CUSTOM_DATA_H

#include "Zeni/Linkage.h"

#include <iosfwd>
#include <memory>

namespace Zeni {

  namespace Rete {

    class Node;

    class ZENI_RETE_LINKAGE Custom_Data {
      Custom_Data(const Custom_Data &) = delete;
      Custom_Data & operator=(const Custom_Data &) = delete;

    public:
      Custom_Data();

      virtual ~Custom_Data();

      virtual Custom_Data * clone() const = 0;

      virtual int64_t rank() const = 0;
      virtual std::shared_ptr<Node> cluster_root_ancestor() const = 0;

      virtual void print_flags(std::ostream &os) const = 0;
      virtual void print_action(std::ostream &os) const = 0;
      virtual std::shared_ptr<const Node> get_suppress() const = 0;
    };

  }

}

#endif
