#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "modules/meta/input_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "x11/events.hpp"
#include "x11/window.hpp"
#include "x11/xkb.hpp"

POLYBAR_NS

class connection;

namespace modules {
  /**
   * Keyboard module using the X keyboard extension
   */
  class xkeyboard_module
      : public static_module<xkeyboard_module>,
        public xpp::event::sink<evt::xkb_new_keyboard_notify, evt::xkb_state_notify, evt::xkb_indicator_state_notify>,
        public input_handler {
   public:
    explicit xkeyboard_module(const bar_settings& bar, string name_);

    void teardown();
    void update();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool query_keyboard();
    bool blacklisted(const string& indicator_name);

    void handle(const evt::xkb_new_keyboard_notify& evt);
    void handle(const evt::xkb_state_notify& evt);
    void handle(const evt::xkb_indicator_state_notify& evt);

    bool on(const input_event_t&);

   private:
    static constexpr const char* TAG_LABEL_LAYOUT{"<label-layout>"};
    static constexpr const char* TAG_LABEL_INDICATOR{"<label-indicator>"};
    static constexpr const char* FORMAT_DEFAULT{"<label-layout> <label-indicator>"};

    static constexpr const char* EVENT_SWITCH{"xkeyboard/switch"};

    connection& m_connection;
    event_timer m_xkbnotify{};
    unique_ptr<keyboard> m_keyboard;

    label_t m_layout;
    label_t m_indicator;
    map<keyboard::indicator::type, label_t> m_indicators;

    vector<string> m_blacklist;
  };
}

POLYBAR_NS_END
