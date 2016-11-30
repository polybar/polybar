#include "x11/connection.hpp"
#include "x11/extensions.hpp"

#include "x11/registry.hpp"

POLYBAR_NS

registry::registry(connection& conn) : xpp_registry(conn) {}

POLYBAR_NS_END
