#pragma once

#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"
#include "utils/bspwm.hpp"

POLYBAR_NS

namespace modules {
  class bspwm_module : public event_module<bspwm_module>, public input_handler {
   public:
    enum class state {
      NONE = 0U,
      EMPTY,
      OCCUPIED,
      FOCUSED,
      URGENT,
      DIMMED,  // used when the monitor is out of focus
    };

    enum class mode {
      NONE = 0U,
      LAYOUT_MONOCLE,
      LAYOUT_TILED,
      STATE_FULLSCREEN,
      STATE_FLOATING,
      STATE_PSEUDOTILED,
      NODE_LOCKED,
      NODE_STICKY,
      NODE_PRIVATE,
      NODE_MARKED
    };

    struct bspwm_monitor {
      vector<pair<unsigned int, label_t>> workspaces;
      vector<label_t> modes;
      label_t label;
      string name;
      bool focused{false};
    };

   public:
    explicit bspwm_module(const bar_settings&, string);

    void stop();
    bool has_event();
    bool update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool input(string&& cmd);

   private:
    bool handle_status(string& data);

    static constexpr auto DEFAULT_ICON = "ws-icon-default";
    static constexpr auto DEFAULT_LABEL = "%icon% %name%";
    static constexpr auto DEFAULT_MONITOR_LABEL = "%name%";

    static constexpr auto TAG_LABEL_MONITOR = "<label-monitor>";
    static constexpr auto TAG_LABEL_STATE = "<label-state>";
    static constexpr auto TAG_LABEL_MODE = "<label-mode>";

    static constexpr const char* EVENT_PREFIX{"bspwm-desk"};
    static constexpr const char* EVENT_CLICK{"bspwm-deskfocus"};
    static constexpr const char* EVENT_SCROLL_UP{"bspwm-desknext"};
    static constexpr const char* EVENT_SCROLL_DOWN{"bspwm-deskprev"};

    bspwm_util::connection_t m_subscriber;

    vector<unique_ptr<bspwm_monitor>> m_monitors;

    map<mode, label_t> m_modelabels;
    map<unsigned int, label_t> m_statelabels;
    label_t m_monitorlabel;
    iconset_t m_icons;

    /**
     * Separator that is inserted in between workspaces
     */
    label_t m_labelseparator;

    bool m_click{true};
    bool m_scroll{true};
    bool m_revscroll{true};
    bool m_pinworkspaces{true};
    bool m_inlinemode{false};
    string_util::hash_type m_hash{0U};
    bool m_fuzzy_match{false};

    // used while formatting output
    size_t m_index{0U};
  };
}

POLYBAR_NS_END
