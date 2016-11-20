#pragma once

#include "config.hpp"

// fwd
#ifdef ENABLE_RANDR_EXT
namespace xpp {
  namespace randr {
    class extension;
  }
}
#endif

#include <xpp/xpp.hpp>

#include "common.hpp"

POLYBAR_NS

class connection;

using gcontext = xpp::gcontext<connection&>;
using pixmap = xpp::pixmap<connection&>;
using drawable = xpp::drawable<connection&>;
using colormap = xpp::colormap<connection&>;
using atom = xpp::atom<connection&>;
using font = xpp::font<connection&>;
using cursor = xpp::cursor<connection&>;

using registry = xpp::event::registry<connection&,
#ifdef ENABLE_DAMAGE_EXT
    xpp::damage::extension
#endif
#ifdef ENABLE_RANDR_EXT
#ifdef ENABLE_DAMAGE_EXT
    ,
#endif
    xpp::randr::extension
#endif
#ifdef ENABLE_RENDER_EXT
#ifdef ENABLE_RANDR_EXT
    ,
#endif
    xpp::render::extension
#endif
    >;

POLYBAR_NS_END
