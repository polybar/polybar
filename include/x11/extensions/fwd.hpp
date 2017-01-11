#pragma once

#include "settings.hpp"

namespace xpp {
#if WITH_XDAMAGE
  namespace damage {
    class extension;
  }
#endif
#if WITH_XRENDER
  namespace render {
    class extension;
  }
#endif
#if WITH_XRANDR
  namespace randr {
    class extension;
  }
#endif
#if WITH_XSYNC
  namespace sync {
    class extension;
  }
#endif
#if WITH_XCOMPOSITE
  namespace composite {
    class extension;
  }
#endif
#if WITH_XKB
  namespace xkb {
    class extension;
  }
#endif
}
