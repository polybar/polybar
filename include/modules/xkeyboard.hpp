#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "drawtypes/layouticonset.hpp"
#include "modules/meta/event_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
#include "x11/extensions/xkb.hpp"
#include "x11/window.hpp"

POLYBAR_NS

class connection;

namespace modules {
  /**
   * Keyboard module using the X keyboard extension
   */
  class xkeyboard_module
      : public static_module<xkeyboard_module>,
        public event_handler<evt::xkb_new_keyboard_notify, evt::xkb_state_notify, evt::xkb_indicator_state_notify> {
   public:
    explicit xkeyboard_module(const bar_settings& bar, string name_, const config&);

    string get_output();
    void update();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = XKEYBOARD_TYPE;

    static constexpr const char* EVENT_SWITCH = "switch";

   protected:
    bool query_keyboard();
    bool blacklisted(const string& indicator_name);

    void handle(const evt::xkb_new_keyboard_notify& evt) override;
    void handle(const evt::xkb_state_notify& evt) override;
    void handle(const evt::xkb_indicator_state_notify& evt) override;

    void action_switch();

    void define_layout_icon(const string& entry, const string& layout, const string& variant, label_t&& icon);
    void parse_icons();

   private:
    static constexpr const char* TAG_LABEL_LAYOUT{"<label-layout>"};
    static constexpr const char* TAG_LABEL_INDICATOR{"<label-indicator>"};
    static constexpr const char* FORMAT_DEFAULT{"<label-layout> <label-indicator>"};
    static constexpr const char* DEFAULT_LAYOUT_ICON{"layout-icon-default"};
    static constexpr const char* DEFAULT_INDICATOR_ICON{"indicator-icon-default"};

    connection& m_connection;
    event_timer m_xkb_newkb_notify{};
    event_timer m_xkb_state_notify{};
    event_timer m_xkb_indicator_notify{};
    unique_ptr<keyboard> m_keyboard;

    label_t m_layout;
    label_t m_indicator_state_on;
    label_t m_indicator_state_off;
    map<keyboard::indicator::type, label_t> m_indicators;
    map<keyboard::indicator::type, label_t> m_indicator_on_labels;
    map<keyboard::indicator::type, label_t> m_indicator_off_labels;

    vector<string> m_blacklist;
    layouticonset_t m_layout_icons;
    iconset_t m_indicator_icons_on;
    iconset_t m_indicator_icons_off;
  };
}  // namespace modules

POLYBAR_NS_END
