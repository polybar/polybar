#include <xpp/event.hpp>

#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"
#include "x11/registry.hpp"

POLYBAR_NS

registry::registry(connection& conn) : xpp::event::registry<connection&, XPP_EXTENSION_LIST>(conn) {}

POLYBAR_NS_END
