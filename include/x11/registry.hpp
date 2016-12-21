#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "x11/extensions/all.hpp"

// fwd
namespace xpp {
  namespace event {
    template <typename Connection, typename... Extensions>
    class registry;
  }
}

POLYBAR_NS

// fwd
class connection;

using xpp_registry = xpp::event::registry<connection&, XPP_EXTENSION_LIST>;

class registry : public xpp_registry {
 public:
  explicit registry(connection& conn);
};

POLYBAR_NS_END
