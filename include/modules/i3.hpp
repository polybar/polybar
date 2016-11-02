#pragma once

#include <i3ipc++/ipc.hpp>

#include "components/config.hpp"
#include "config.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"
#include "utils/i3.hpp"
#include "utils/io.hpp"

LEMONBUDDY_NS

namespace modules {
  // meta types {{{

  enum class i3_flag {
    WORKSPACE_NONE,
    WORKSPACE_FOCUSED,
    WORKSPACE_UNFOCUSED,
    WORKSPACE_VISIBLE,
    WORKSPACE_URGENT,
  };

  struct i3_workspace {
    int index;
    i3_flag flag;
    label_t label;

    i3_workspace(int index_, i3_flag flag_, label_t&& label_)
        : index(index_), flag(flag_), label(forward<decltype(label_)>(label_)) {}

    operator bool() {
      return label && *label;
    }
  };

  using i3_workspace_t = unique_ptr<i3_workspace>;

  // }}}

  class i3_module : public event_module<i3_module> {
   public:
    using event_module::event_module;

    void setup();
    void stop();
    bool has_event();
    bool update();
    bool build(builder* builder, string tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

   private:
    static constexpr auto DEFAULT_WS_ICON = "ws-icon-default";
    static constexpr auto DEFAULT_WS_LABEL = "%icon% %name%";
    static constexpr auto TAG_LABEL_STATE = "<label-state>";

    static constexpr auto EVENT_PREFIX = "i3";
    static constexpr auto EVENT_CLICK = "i3-wsfocus-";
    static constexpr auto EVENT_SCROLL_UP = "i3-wsnext";
    static constexpr auto EVENT_SCROLL_DOWN = "i3-wsprev";

    map<i3_flag, label_t> m_statelabels;
    vector<i3_workspace_t> m_workspaces;
    iconset_t m_icons;

    bool m_indexsort = false;
    bool m_pinworkspaces = false;
    bool m_strip_wsnumbers = false;
    size_t m_wsname_maxlen = 0;

    i3_util::connection_t m_ipc;
  };
}

LEMONBUDDY_NS_END
