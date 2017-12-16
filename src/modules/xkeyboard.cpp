#include "modules/xkeyboard.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "utils/factory.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<xkeyboard_module>;

  /**
   * Construct module
   */
  xkeyboard_module::xkeyboard_module(const bar_settings& bar, string name_)
      : static_module<xkeyboard_module>(bar, move(name_)), m_connection(connection::make()) {
    // Load config values
    m_blacklist = m_conf.get_list(name(), "blacklist", {});

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, FORMAT_DEFAULT, {TAG_LABEL_LAYOUT, TAG_LABEL_INDICATOR});

    if (m_formatter->has(TAG_LABEL_LAYOUT)) {
      m_layout = load_optional_label(m_conf, name(), TAG_LABEL_LAYOUT, "%layout%");
    }
    if (m_formatter->has(TAG_LABEL_INDICATOR)) {
      m_indicator = load_optional_label(m_conf, name(), TAG_LABEL_INDICATOR, "%name%");
    }

    // Setup extension
    // clang-format off
    m_connection.xkb().select_events_checked(XCB_XKB_ID_USE_CORE_KBD,
        XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_STATE_NOTIFY | XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY, 0,
        XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_STATE_NOTIFY | XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY, 0, 0, nullptr);
    // clang-format on

    // Create keyboard object
    query_keyboard();
  }

  /**
   * Update labels with extension data
   */
  void xkeyboard_module::update() {
    if (m_layout) {
      m_layout->reset_tokens();
      m_layout->replace_token("%name%", m_keyboard->group_name(m_keyboard->current()));
      m_layout->replace_token("%layout%", m_keyboard->layout_name(m_keyboard->current()));
      m_layout->replace_token("%number%", to_string(m_keyboard->current()));
    }

    if (m_indicator) {
      m_indicators.clear();

      const auto& caps = keyboard::indicator::type::CAPS_LOCK;
      const auto& caps_str = m_keyboard->indicator_name(caps);

      if (!blacklisted(caps_str) && m_keyboard->on(caps)) {
        m_indicators[caps] = m_indicator->clone();
        m_indicators[caps]->replace_token("%name%", caps_str);
      }

      const auto& num = keyboard::indicator::type::NUM_LOCK;
      const auto& num_str = m_keyboard->indicator_name(num);

      if (!blacklisted(num_str) && m_keyboard->on(num)) {
        m_indicators[num] = m_indicator->clone();
        m_indicators[num]->replace_token("%name%", num_str);
      }
    }

    // Trigger redraw
    broadcast();
  }

  /**
   * Build module output and wrap it in a click handler use
   * to cycle between configured layout groups
   */
  string xkeyboard_module::get_output() {
    string output{module::get_output()};

    if (m_keyboard && m_keyboard->size() > 1) {
      m_builder->cmd(mousebtn::LEFT, EVENT_SWITCH);
      m_builder->append(output);
      m_builder->cmd_close();
    } else {
      m_builder->append(output);
    }

    return m_builder->flush();
  }

  /**
   * Map format tags to content
   */
  bool xkeyboard_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_LAYOUT) {
      builder->node(m_layout);
    } else if (tag == TAG_LABEL_INDICATOR && !m_indicators.empty()) {
      size_t n{0};
      for (auto&& indicator : m_indicators) {
        if (n++) {
          builder->space(m_formatter->get(DEFAULT_FORMAT)->spacing);
        }
        builder->node(indicator.second);
      }
      return n > 0;
    } else {
      return false;
    }

    return true;
  }

  /**
   * Handle input command
   */
  bool xkeyboard_module::input(string&& cmd) {
    if (cmd.compare(0, strlen(EVENT_SWITCH), EVENT_SWITCH) != 0) {
      return false;
    }

    size_t current_group = m_keyboard->current() + 1;

    if (current_group >= m_keyboard->size()) {
      current_group = 0;
    }

    xkb_util::switch_layout(m_connection, XCB_XKB_ID_USE_CORE_KBD, current_group);
    m_keyboard->current(current_group);
    m_connection.flush();

    update();

    return true;
  }

  /**
   * Create keyboard object by querying current extension data
   */
  bool xkeyboard_module::query_keyboard() {
    try {
      auto layouts = xkb_util::get_layouts(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      auto indicators = xkb_util::get_indicators(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      auto current_group = xkb_util::get_current_group(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      m_keyboard = factory_util::unique<keyboard>(move(layouts), move(indicators), current_group);
      return true;
    } catch (const exception& err) {
      throw module_error("Failed to query keyboard, err: " + string{err.what()});
    }

    return false;
  }

  /**
   * Check if the indicator has been blacklisted by the user
   */
  bool xkeyboard_module::blacklisted(const string& indicator_name) {
    for (auto&& i : m_blacklist) {
      if (string_util::compare(i, indicator_name)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Handler for XCB_XKB_NEW_KEYBOARD_NOTIFY events
   */
  void xkeyboard_module::handle(const evt::xkb_new_keyboard_notify& evt) {
    if (evt->changed & XCB_XKB_NKN_DETAIL_KEYCODES && m_xkb_newkb_notify.allow(evt->time)) {
      query_keyboard();
      update();
    }
  }

  /**
   * Handler for XCB_XKB_STATE_NOTIFY events
   */
  void xkeyboard_module::handle(const evt::xkb_state_notify& evt) {
    if (m_keyboard && evt->changed & XCB_XKB_STATE_PART_GROUP_STATE && m_xkb_state_notify.allow(evt->time)) {
      m_keyboard->current(evt->group);
      update();
    }
  }

  /**
   * Handler for XCB_XKB_INDICATOR_STATE_NOTIFY events
   */
  void xkeyboard_module::handle(const evt::xkb_indicator_state_notify& evt) {
    if (m_keyboard && m_xkb_indicator_notify.allow(evt->time)) {
      m_keyboard->set(m_connection.xkb().get_state(XCB_XKB_ID_USE_CORE_KBD)->lockedMods);
      update();
    }
  }
}

POLYBAR_NS_END
