#include "modules/xkeyboard.hpp"

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace modules {
  template class module<xkeyboard_module>;

  // clang-format off
  static const keyboard::indicator::type INDICATOR_TYPES[] {
    keyboard::indicator::type::CAPS_LOCK,
    keyboard::indicator::type::NUM_LOCK,
    keyboard::indicator::type::SCROLL_LOCK
  };
  // clang-format on

  /**
   * Construct module
   */
  xkeyboard_module::xkeyboard_module(const bar_settings& bar, string name_, const config& config)
      : static_module<xkeyboard_module>(bar, move(name_), config), m_connection(connection::make()) {
    m_router->register_action(EVENT_SWITCH, [this]() { action_switch(); });

    // Setup extension
    // clang-format off
    m_connection.xkb().select_events_checked(XCB_XKB_ID_USE_CORE_KBD,
      XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
      XCB_XKB_EVENT_TYPE_STATE_NOTIFY |
      XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY, 0,
      XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
      XCB_XKB_EVENT_TYPE_STATE_NOTIFY |
      XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY, 0, 0, nullptr);
    // clang-format on

    // Create keyboard object
    query_keyboard();

    // Load config values
    m_blacklist = m_conf.get_list(name(), "blacklist", {});

    // load layout icons
    parse_icons();

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, FORMAT_DEFAULT, {TAG_LABEL_LAYOUT, TAG_LABEL_INDICATOR});

    if (m_formatter->has(TAG_LABEL_LAYOUT)) {
      m_layout = load_optional_label(m_conf, name(), TAG_LABEL_LAYOUT, "%layout%");
    }

    if (m_formatter->has(TAG_LABEL_INDICATOR)) {
      m_conf.warn_deprecated(name(), "label-indicator", "label-indicator-on");
      // load an empty label if 'label-indicator-off' is not explicitly specified so
      // no existing user configs are broken (who expect nothing to be shown when indicator is off)
      m_indicator_state_off = load_optional_label(m_conf, name(), "label-indicator-off"s, ""s);

      if (m_conf.has(name(), "label-indicator-on"s)) {
        m_indicator_state_on = load_optional_label(m_conf, name(), "label-indicator-on"s, "%name%"s);
      } else {
        // if 'label-indicator-on' is not explicitly specified, use 'label-indicator'
        // as to not break existing user configs
        m_indicator_state_on = load_optional_label(m_conf, name(), TAG_LABEL_INDICATOR, "%name%"s);
      }

      // load indicator icons
      m_indicator_icons_off = std::make_shared<iconset>();
      m_indicator_icons_on = std::make_shared<iconset>();

      auto icon_pair = string_util::tokenize(m_conf.get(name(), DEFAULT_INDICATOR_ICON, ""s), ';');
      if (icon_pair.size() == 2) {
        m_indicator_icons_off->add(DEFAULT_INDICATOR_ICON, std::make_shared<label>(icon_pair[0]));
        m_indicator_icons_on->add(DEFAULT_INDICATOR_ICON, std::make_shared<label>(icon_pair[1]));
      } else {
        m_indicator_icons_off->add(DEFAULT_INDICATOR_ICON, std::make_shared<label>(""s));
        m_indicator_icons_on->add(DEFAULT_INDICATOR_ICON, std::make_shared<label>(""s));
      }

      for (const auto& it : m_conf.get_list<string>(name(), "indicator-icon", {})) {
        auto icon_triple = string_util::tokenize(it, ';');
        if (icon_triple.size() == 3) {
          auto const indicator_str = string_util::lower(icon_triple[0]);
          m_indicator_icons_off->add(indicator_str, std::make_shared<label>(icon_triple[1]));
          m_indicator_icons_on->add(indicator_str, std::make_shared<label>(icon_triple[2]));
        }
      }

      for (auto it : INDICATOR_TYPES) {
        const auto& indicator_str = m_keyboard->indicator_name(it);
        auto key_name = string_util::replace(string_util::lower(indicator_str), " "s, ""s);
        const auto indicator_key_on = "label-indicator-on-"s + key_name;
        const auto indicator_key_off = "label-indicator-off-"s + key_name;

        if (m_conf.has(name(), indicator_key_on)) {
          m_indicator_on_labels.emplace(it, load_label(m_conf, name(), indicator_key_on));
        }
        if (m_conf.has(name(), indicator_key_off)) {
          m_indicator_off_labels.emplace(it, load_label(m_conf, name(), indicator_key_off));
        }
      }
    }
  }

  /**
   * Update labels with extension data
   */
  void xkeyboard_module::update() {
    if (m_layout) {
      m_layout->reset_tokens();
      m_layout->replace_token("%name%", m_keyboard->group_name(m_keyboard->current()));
      m_layout->replace_token("%variant%", m_keyboard->variant_name(m_keyboard->current()));

      auto const current_layout = m_keyboard->layout_name(m_keyboard->current());
      auto const current_variant = m_keyboard->variant_name(m_keyboard->current());

      auto icon = m_layout_icons->get(current_layout, current_variant);

      m_layout->replace_token("%icon%", icon->get());
      m_layout->replace_token("%layout%", current_layout);
      m_layout->replace_token("%number%", to_string(m_keyboard->current()));
    }

    if (m_formatter->has(TAG_LABEL_INDICATOR)) {
      m_indicators.clear();

      for (auto it : INDICATOR_TYPES) {
        const auto& indicator_str = m_keyboard->indicator_name(it);

        if (blacklisted(indicator_str)) {
          continue;
        }

        auto indicator_on = m_keyboard->on(it);
        auto& indicator_labels = indicator_on ? m_indicator_on_labels : m_indicator_off_labels;
        auto& indicator_icons = indicator_on ? m_indicator_icons_on : m_indicator_icons_off;
        auto& indicator_state = indicator_on ? m_indicator_state_on : m_indicator_state_off;

        label_t indicator;
        if (indicator_labels.find(it) != indicator_labels.end()) {
          indicator = indicator_labels[it]->clone();
        } else {
          indicator = indicator_state->clone();
        }

        auto icon = indicator_icons->get(string_util::lower(indicator_str), DEFAULT_INDICATOR_ICON);

        indicator->replace_token("%name%", indicator_str);
        indicator->replace_token("%icon%", icon->get());
        m_indicators.emplace(it, move(indicator));
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
      m_builder->action(mousebtn::LEFT, *this, EVENT_SWITCH, "");
      m_builder->node(output);
      m_builder->action_close();
    } else {
      m_builder->node(output);
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
        if (*indicator.second) {
          if (n++) {
            builder->spacing(m_formatter->get(DEFAULT_FORMAT)->spacing);
          }
          builder->node(indicator.second);
        }
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
  void xkeyboard_module::action_switch() {
    size_t current_group = m_keyboard->current() + 1;

    if (current_group >= m_keyboard->size()) {
      current_group = 0;
    }

    xkb_util::switch_layout(m_connection, XCB_XKB_ID_USE_CORE_KBD, current_group);
    m_keyboard->current(current_group);
    m_connection.flush();

    update();
  }

  /**
   * Create keyboard object by querying current extension data
   */
  bool xkeyboard_module::query_keyboard() {
    try {
      auto layouts = xkb_util::get_layouts(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      auto indicators = xkb_util::get_indicators(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      auto current_group = xkb_util::get_current_group(m_connection, XCB_XKB_ID_USE_CORE_KBD);
      m_keyboard = std::make_unique<keyboard>(move(layouts), move(indicators), current_group);
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

  void xkeyboard_module::parse_icons() {
    m_layout_icons = make_shared<layouticonset>(load_optional_label(m_conf, name(), DEFAULT_LAYOUT_ICON, ""s));

    for (const auto& it : m_conf.get_list<string>(name(), "layout-icon", {})) {
      auto vec = string_util::tokenize(it, ';');

      size_t size = vec.size();
      if (size != 2 && size != 3) {
        m_log.warn("%s: Malformed layout-icon '%s'", name(), it);
        continue;
      }

      const string& layout = vec[0];

      if (layout.empty()) {
        m_log.warn("%s: layout-icon '%s' is invalid: there must always be a layout defined", name(), it);
        continue;
      }

      const string& variant = size == 2 ? layouticonset::VARIANT_ANY : vec[1];
      const string& icon = vec.back();

      if (layout == layouticonset::VARIANT_ANY && variant == layouticonset::VARIANT_ANY) {
        m_log.warn("%s: Using '%s' for layout-icon means declaring a default icon, use 'layout-icon-default' instead",
            name(), it);
        continue;
      }

      define_layout_icon(it, layout, variant, std::make_shared<label>(icon));
    }
  }

  void xkeyboard_module::define_layout_icon(
      const string& entry, const string& layout, const string& variant, label_t&& icon) {
    if (m_layout_icons->contains(layout, variant)) {
      m_log.warn(
          "%s: An equivalent matching is already defined for '%s;%s' => ignoring '%s'", name(), layout, variant, entry);
    } else if (!m_layout_icons->add(layout, variant, std::forward<label_t>(icon))) {
      m_log.err(
          "%s: '%s' cannot be added to internal structure. This case should never happen and must be reported as a bug",
          name(), entry);
    }
  }
} // namespace modules

POLYBAR_NS_END
