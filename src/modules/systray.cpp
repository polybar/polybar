#if DEBUG
#include "modules/systray.hpp"
#include "drawtypes/label.hpp"
#include "x11/connection.hpp"
#include "x11/tray_manager.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<systray_module>;

  /**
   * Construct module
   */
  systray_module::systray_module(const bar_settings& bar, string name_)
      : static_module<systray_module>(bar, move(name_)), m_connection(connection::make()) {
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
      builder->cmd(mousebtn::LEFT, EVENT_TOGGLE);
      builder->node(m_label);
      builder->cmd_close();
    } else if (tag == TAG_TRAY_CLIENTS && !m_hidden) {
      builder->append(TRAY_PLACEHOLDER);
    } else {
      return false;
    }
    return true;
  }

  /**
   * Handle input event
   */
  bool systray_module::input(string&& cmd) {
    if (cmd.find(EVENT_TOGGLE) != 0) {
      return false;
    }

    m_hidden = !m_hidden;
    broadcast();

    return true;
  }
}

POLYBAR_NS_END
#endif
