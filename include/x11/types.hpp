#pragma once

#include "config.hpp"

// fwd
namespace xpp {
#if ENABLE_DAMAGE_EXT
  namespace damage {
    class extension;
  }
#endif
#if ENABLE_RANDR_EXT
  namespace randr {
    class extension;
  }
#endif
#if ENABLE_SYNC_EXT
  namespace sync {
    class extension;
  }
#endif
#if ENABLE_RENDER_EXT
  namespace render {
    class extension;
  }
#endif
#if ENABLE_COMPOSITE_EXT
  namespace composite {
    class extension;
  }
#endif
}

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
#if ENABLE_DAMAGE_EXT
    xpp::damage::extension
#endif
#if ENABLE_RANDR_EXT
#if ENABLE_DAMAGE_EXT
    ,
#endif
    xpp::randr::extension
#endif
#if ENABLE_RENDER_EXT
#if (ENABLE_RANDR_EXT || ENABLE_DAMAGE_EXT)
    ,
#endif
    xpp::render::extension
#endif
#if ENABLE_SYNC_EXT
#if (ENABLE_RANDR_EXT || ENABLE_DAMAGE_EXT || ENABLE_RENDER_EXT)
    ,
#endif
    xpp::sync::extension
#endif
#if ENABLE_COMPOSITE_EXT
#if (ENABLE_RANDR_EXT || ENABLE_DAMAGE_EXT || ENABLE_RENDER_EXT || ENABLE_COMPOSITE_EXT)
    ,
#endif
    xpp::composite::extension
#endif
    >;

POLYBAR_NS_END
