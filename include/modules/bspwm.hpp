#pragma once

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"
#include "utils/bspwm.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class state_ws {
    WORKSPACE_NONE,
    WORKSPACE_ACTIVE,
    WORKSPACE_URGENT,
    WORKSPACE_EMPTY,
    WORKSPACE_OCCUPIED,
    WORKSPACE_DIMMED,  // used when the monitor is out of focus
  };
  enum class state_mode {
    MODE_NONE,
    MODE_LAYOUT_MONOCLE,
    MODE_LAYOUT_TILED,
    MODE_STATE_FULLSCREEN,
    MODE_STATE_FLOATING,
    MODE_NODE_LOCKED,
    MODE_NODE_STICKY,
    MODE_NODE_PRIVATE
  };

  struct bspwm_monitor {
    vector<pair<state_ws, label_t>> workspaces;
    vector<label_t> modes;
    label_t label;
    string name;
    bool focused = false;
  };

  class bspwm_module : public event_module<bspwm_module> {
   public:
    using event_module::event_module;

    void setup();
    void stop();
    bool has_event();
    bool update();
    string get_output();
    bool build(builder* builder, string tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

   private:
    static constexpr auto DEFAULT_WS_ICON = "ws-icon-default";
    static constexpr auto DEFAULT_WS_LABEL = "%icon% %name%";
    static constexpr auto DEFAULT_MONITOR_LABEL = "%name%";
    static constexpr auto TAG_LABEL_MONITOR = "<label-monitor>";
    static constexpr auto TAG_LABEL_STATE = "<label-state>";
    static constexpr auto TAG_LABEL_MODE = "<label-mode>";
    static constexpr auto EVENT_CLICK = "bwm";

    bspwm_util::connection_t m_subscriber;

    vector<unique_ptr<bspwm_monitor>> m_monitors;

    map<state_mode, label_t> m_modelabels;
    map<state_ws, label_t> m_statelabels;
    label_t m_monitorlabel;
    iconset_t m_icons;
    bool m_pinworkspaces = true;
    unsigned long m_hash;

    // used while formatting output
    size_t m_index = 0;
  };
}

LEMONBUDDY_NS_END
