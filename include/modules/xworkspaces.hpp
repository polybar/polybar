#pragma once

#include <bitset>

#include "components/config.hpp"
#include "components/types.hpp"
#include "modules/meta/event_handler.hpp"
#include "modules/meta/input_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"
#include "x11/window.hpp"

POLYBAR_NS

class connection;

namespace modules {
  enum class desktop_state {
    NONE,
    ACTIVE,
    URGENT,
    EMPTY,
    OCCUPIED,
  };

  enum class viewport_state {
    NONE,
    FOCUSED,
    UNFOCUSED,
  };

  struct desktop {
    explicit desktop(unsigned int index, unsigned int offset, desktop_state state, label_t&& label)
        : index(index), offset(offset), state(state), label(label) {}
    unsigned int index;
    unsigned int offset;
    desktop_state state;
    label_t label;
  };

  struct viewport {
    position pos;
    string name;
    vector<unique_ptr<desktop>> desktops;
    viewport_state state;
    label_t label;
  };

  /**
   * Module used to display EWMH desktops
   */
  class xworkspaces_module : public static_module<xworkspaces_module>,
                             public event_handler<evt::property_notify>,
                             public input_handler {
   public:
    explicit xworkspaces_module(const bar_settings& bar, string name_);

    void update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    void handle(const evt::property_notify& evt);

    void rebuild_clientlist();
    void rebuild_desktops();
    void rebuild_desktop_states();
    void set_desktop_urgent(xcb_window_t window);

    bool input(string&& cmd);
    vector<string> get_desktop_names();

   private:
    static constexpr const char* DEFAULT_ICON{"icon-default"};
    static constexpr const char* DEFAULT_LABEL_STATE{"%icon% %name%"};
    static constexpr const char* DEFAULT_LABEL_MONITOR{"%name%"};

    static constexpr const char* TAG_LABEL_MONITOR{"<label-monitor>"};
    static constexpr const char* TAG_LABEL_STATE{"<label-state>"};

    static constexpr const char* EVENT_PREFIX{"xworkspaces-"};
    static constexpr const char* EVENT_CLICK{"focus="};
    static constexpr const char* EVENT_SCROLL_UP{"next"};
    static constexpr const char* EVENT_SCROLL_DOWN{"prev"};

    connection& m_connection;
    ewmh_connection_t m_ewmh;

    vector<monitor_t> m_monitors;
    bool m_monitorsupport{true};

    vector<string> m_desktop_names;
    unsigned int m_current_desktop;
    string m_current_desktop_name;

    vector<xcb_window_t> m_clientlist;
    vector<unique_ptr<viewport>> m_viewports;
    map<desktop_state, label_t> m_labels;
    label_t m_monitorlabel;
    iconset_t m_icons;
    bool m_pinworkspaces{false};
    bool m_click{true};
    bool m_scroll{true};
    size_t m_index{0};

    event_timer m_timer{0L, 25L};
  };
}

POLYBAR_NS_END
