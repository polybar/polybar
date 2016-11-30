#pragma once

#include "common.hpp"

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
#if ENABLE_XKB_EXT
  namespace xkb {
    class extension;
  }
#endif
}
