#pragma once

#include "common.hpp"
#include "x11/extensions/fwd.hpp"

// fwd
namespace xpp {
  namespace event {
    template <typename Connection, typename... Extensions>
    class registry;
  }
}

POLYBAR_NS

class connection;

class registry : public xpp::event::registry<connection&, XPP_EXTENSION_LIST> {
 public:
  using priority = unsigned int;

  explicit registry(connection& conn);
};

POLYBAR_NS_END
