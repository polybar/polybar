#pragma once

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"
#include "utils/bspwm.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class bspwm_flag {
    WORKSPACE_NONE,
    WORKSPACE_ACTIVE,
    WORKSPACE_URGENT,
    WORKSPACE_EMPTY,
    WORKSPACE_OCCUPIED,
    WORKSPACE_DIMMED,  // used when the monitor is out of focus
    MODE_NONE,
    MODE_LAYOUT_MONOCLE,
    MODE_LAYOUT_TILED,
    MODE_STATE_FULLSCREEN,
    MODE_STATE_FLOATING,
    MODE_NODE_LOCKED,
    MODE_NODE_STICKY,
    MODE_NODE_PRIVATE
  };

  struct bspwm_workspace {
    bspwm_flag flag;
    label_t label;

    bspwm_workspace(bspwm_flag flag, label_t&& label)
        : flag(flag), label(forward<decltype(label)>(label)) {}

    operator bool() {
      return label && *label;
    }
  };

  using bspwm_workspace_t = unique_ptr<bspwm_workspace>;

  class bspwm_module : public event_module<bspwm_module> {
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
    static constexpr auto TAG_LABEL_MODE = "<label-mode>";
    static constexpr auto EVENT_CLICK = "bwm";

    bspwm_util::connection_t m_subscriber;

    map<bspwm_flag, label_t> m_modelabels;
    map<bspwm_flag, label_t> m_statelabels;
    vector<bspwm_workspace_t> m_workspaces;
    vector<label_t> m_modes;
    iconset_t m_icons;
    string m_monitor;
    unsigned long m_hash;
  };
}

LEMONBUDDY_NS_END
