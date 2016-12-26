#pragma once

#include "common.hpp"

namespace xpp {
  template <typename Connection, template <typename, typename> class...>
  class gcontext;
  template <typename Connection, template <typename, typename> class...>
  class pixmap;
  template <typename Connection, template <typename, typename> class...>
  class drawable;
  template <typename Connection, template <typename, typename> class...>
  class colormap;
  template <typename Connection, template <typename, typename> class...>
  class atom;
  template <typename Connection, template <typename, typename> class...>
  class font;
  template <typename Connection, template <typename, typename> class...>
  class cursor;
  namespace event {
    template <class Event, class... Events>
    class sink;
  }
}

POLYBAR_NS

class connection;
class registry;

using gcontext = xpp::gcontext<connection&>;
using pixmap = xpp::pixmap<connection&>;
using drawable = xpp::drawable<connection&>;
using colormap = xpp::colormap<connection&>;
using atom = xpp::atom<connection&>;
using font = xpp::font<connection&>;
using cursor = xpp::cursor<connection&>;

POLYBAR_NS_END
