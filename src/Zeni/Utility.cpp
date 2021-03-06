#ifndef ZENI_RETE_UTILITY_H
#define ZENI_RETE_UTILITY_H

#include <iomanip>
#include <sstream>

namespace Zeni {

  std::string to_string(const double &number) noexcept {
    std::ostringstream oss;
    oss << std::setprecision(20) << std::fixed << number;
    std::string str = oss.str();
    size_t len = str.length();
    while (str[len - 1] == '0' && str[len - 2] != '.')
      --len;
    return str.substr(0, len);
  }

}


#endif
