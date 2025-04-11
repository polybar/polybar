#pragma once

#include "modules/meta/event_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
#include "x11/ewmh.hpp"

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
    explicit desktop(unsigned int index, desktop_state state, label_t&& label)
        : index(index), state(state), label(label) {}
    unsigned int index;
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
  class xworkspaces_module : public static_module<xworkspaces_module>, public event_handler<evt::property_notify> {
   public:
    explicit xworkspaces_module(const bar_settings& bar, string name_, const config&);

    void update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = XWORKSPACES_TYPE;

    static constexpr auto EVENT_FOCUS = "focus";
    static constexpr auto EVENT_NEXT = "next";
    static constexpr auto EVENT_PREV = "prev";

   protected:
    void handle(const evt::property_notify& evt) override;

    void rebuild_clientlist();
    void rebuild_urgent_hints();
    void rebuild_desktops();
    void rebuild_desktop_states();
    void update_current_desktop();

    void action_focus(const string& data);
    void action_next();
    void action_prev();

    void focus_direction(bool next);
    void focus_desktop(unsigned new_desktop);

   private:
    static vector<string> get_desktop_names();

    static constexpr const char* DEFAULT_ICON{"icon-default"};
    static constexpr const char* DEFAULT_LABEL_STATE{"%icon% %name%"};
    static constexpr const char* DEFAULT_LABEL_MONITOR{"%name%"};

    static constexpr const char* TAG_LABEL_MONITOR{"<label-monitor>"};
    static constexpr const char* TAG_LABEL_STATE{"<label-state>"};

    connection& m_connection;
    ewmh_util::ewmh_connection& m_ewmh;

    vector<monitor_t> m_monitors;

    vector<string> m_desktop_names;
    vector<bool> m_urgent_desktops;
    unsigned int m_current_desktop;

    /**
     * Maps an xcb window to its desktop number
     */
    map<xcb_window_t, unsigned int> m_clients;
    map<unsigned int, unsigned int> m_windows;
    vector<unique_ptr<viewport>> m_viewports;
    map<desktop_state, label_t> m_labels;
    label_t m_monitorlabel;
    iconset_t m_icons;
    bool m_pinworkspaces{false};
    bool m_click{true};
    bool m_scroll{true};
    bool m_revscroll{false};
    bool m_group_by_monitor{true};
    size_t m_index{0};
  };
} // namespace modules

POLYBAR_NS_END
