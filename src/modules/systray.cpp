#if DEBUG
#include "modules/systray.hpp"

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "x11/connection.hpp"
#include "x11/tray_manager.hpp"

POLYBAR_NS

namespace modules {
  template class module<systray_module>;

  /**
   * Construct module
   */
  systray_module::systray_module(const bar_settings& bar, string name_)
      : static_module<systray_module>(bar, move(name_)), m_connection(connection::make()) {
    m_router->register_action(EVENT_TOGGLE, [this]() { action_toggle(); });

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_TOGGLE, {TAG_LABEL_TOGGLE, TAG_TRAY_CLIENTS});

    if (m_formatter->has(TAG_LABEL_TOGGLE)) {
      m_label = load_label(m_conf, name(), TAG_LABEL_TOGGLE);
    }
  }

  /**
   * Update
   */
  void systray_module::update() {
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%title%", "");
    }

    broadcast();
  }

  /**
   * Build output
   */
  bool systray_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_TOGGLE) {
      builder->action(mousebtn::LEFT, *this, EVENT_TOGGLE, "", m_label);
    } else if (tag == TAG_TRAY_CLIENTS && !m_hidden) {
      builder->node(TRAY_PLACEHOLDER);
    } else {
      return false;
    }
    return true;
  }

  /**
   * Handle input event
   */
  void systray_module::action_toggle() {
    m_hidden = !m_hidden;
    broadcast();
  }
} // namespace modules

POLYBAR_NS_END
#endif
