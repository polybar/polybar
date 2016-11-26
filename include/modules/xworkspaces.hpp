#pragma once

#include <bitset>

#include "components/config.hpp"
#include "modules/meta/static_module.hpp"
#include "x11/events.hpp"
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

  struct desktop {
    explicit desktop(string&& name, desktop_state state) : name(name), state(state) {}
    string name;
    desktop_state state{desktop_state::NONE};
    label_t label;
  };

  /**
   * Module used to display EWMH desktops
   */
  class xworkspaces_module : public static_module<xworkspaces_module>, public xpp::event::sink<evt::property_notify> {
   public:
    xworkspaces_module(const bar_settings& bar, const logger& logger, const config& config, string name);

    void setup();
    void teardown();
    void handle(const evt::property_notify& evt);
    void update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    void rebuild_desktops();
    void set_current_desktop();

   private:
    static constexpr const char* DEFAULT_ICON{"icon-default"};
    static constexpr const char* DEFAULT_LABEL{"%icon% %name%"};

    static constexpr const char* TAG_LABEL{"<label>"};

    connection& m_connection;
    ewmh_connection_t m_ewmh;
    event_timer m_throttle{0, 0};

    vector<unique_ptr<desktop>> m_desktops;
    map<desktop_state, label_t> m_labels;
    iconset_t m_icons;

    size_t m_index{0};
  };
}

POLYBAR_NS_END
