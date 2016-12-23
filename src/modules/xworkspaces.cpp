#include <utility>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/xworkspaces.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<xworkspaces_module>;

  /**
   * Construct module
   */
  xworkspaces_module::xworkspaces_module(const bar_settings& bar, string name_)
      : static_module<xworkspaces_module>(bar, move(name_)), m_connection(connection::make()) {
    // Load config values
    m_pinworkspaces = m_conf.get<bool>(name(), "pin-workspaces", m_pinworkspaces);
    m_click = m_conf.get<bool>(name(), "enable-click", m_click);
    m_scroll = m_conf.get<bool>(name(), "enable-scroll", m_scroll);

    // Initialize ewmh atoms
    if ((m_ewmh = ewmh_util::initialize()) == nullptr) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_CURRENT_DESKTOP
    if (!ewmh_util::supports(m_ewmh.get(), m_ewmh->_NET_CURRENT_DESKTOP)) {
      throw module_error("The WM does not support _NET_CURRENT_DESKTOP, aborting...");
    }

    // Check if the WM supports _NET_DESKTOP_VIEWPORT
    if (!ewmh_util::supports(m_ewmh.get(), m_ewmh->_NET_DESKTOP_VIEWPORT) && m_pinworkspaces) {
      throw module_error("The WM does not support _NET_DESKTOP_VIEWPORT (required when `pin-workspaces = true`)");
    } else if (!m_pinworkspaces) {
      m_monitorsupport = false;
    }

    // Get list of monitors
    m_monitors = randr_util::get_monitors(m_connection, m_connection.root(), false);

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE, TAG_LABEL_MONITOR});

    if (m_formatter->has(TAG_LABEL_MONITOR)) {
      m_monitorlabel = load_optional_label(m_conf, name(), "label-monitor", DEFAULT_LABEL_MONITOR);
    }

    if (m_formatter->has(TAG_LABEL_STATE)) {
      // clang-format off
      m_labels.insert(make_pair(
          desktop_state::ACTIVE, load_optional_label(m_conf, name(), "label-active", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::OCCUPIED, load_optional_label(m_conf, name(), "label-occupied", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::EMPTY, load_optional_label(m_conf, name(), "label-empty", DEFAULT_LABEL_STATE)));
      // clang-format on
    }

    m_icons = factory_util::shared<iconset>();
    m_icons->add(DEFAULT_ICON, factory_util::shared<label>(m_conf.get<string>(name(), DEFAULT_ICON, "")));

    for (const auto& workspace : m_conf.get_list<string>(name(), "icon", {})) {
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], factory_util::shared<label>(vec[1]));
      }
    }

    // Make sure we get notified when root properties change
    window{m_connection, m_connection.root()}.ensure_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE);
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xworkspaces_module::handle(const evt::property_notify& evt) {
    if (evt->atom == m_ewmh->_NET_DESKTOP_NAMES) {
      update();
    } else if (evt->atom == m_ewmh->_NET_CURRENT_DESKTOP) {
      update();
    } else {
      return;
    }

    // Emit notification to trigger redraw
    broadcast();
  }

  /**
   * Fetch and parse data
   */
  void xworkspaces_module::update() {
    auto current = ewmh_util::get_current_desktop(m_ewmh.get());
    auto names = ewmh_util::get_desktop_names(m_ewmh.get());
    vector<position> viewports;
    size_t num{0};
    position pos{};

    if (m_monitorsupport) {
      viewports = ewmh_util::get_desktop_viewports(m_ewmh.get());
      num = math_util::min(names.size(), viewports.size());
    } else {
      num = names.size();
    }

    m_viewports.clear();

    for (size_t n = 0; n < num; n++) {
      if (m_pinworkspaces && !m_bar.monitor->match(viewports[n])) {
        continue;
      }

      if (m_monitorsupport && (n == 0 || pos.x != viewports[n].x || pos.y != viewports[n].y)) {
        m_viewports.emplace_back(factory_util::unique<viewport>());

        for (auto&& monitor : m_monitors) {
          if (monitor->match(viewports[n])) {
            m_viewports.back()->name = monitor->name;
            m_viewports.back()->pos.x = static_cast<int16_t>(monitor->x);
            m_viewports.back()->pos.y = static_cast<int16_t>(monitor->y);
            m_viewports.back()->state = viewport_state::FOCUSED;
            m_viewports.back()->label = m_monitorlabel->clone();
            m_viewports.back()->label->replace_token("%name%", monitor->name);
            pos = m_viewports.back()->pos;
            break;
          }
        }
      } else if (!m_monitorsupport && n == 0) {
        m_viewports.emplace_back(factory_util::unique<viewport>());
        m_viewports.back()->state = viewport_state::NONE;
      }

      desktop_state state{current == n ? desktop_state::ACTIVE : desktop_state::EMPTY};
      m_viewports.back()->desktops.emplace_back(factory_util::unique<desktop>(n, state, m_labels[state]->clone()));

      auto& desktop = m_viewports.back()->desktops.back();
      desktop->label->reset_tokens();
      desktop->label->replace_token("%name%", names[n]);
      desktop->label->replace_token("%icon%", m_icons->get(names[n], DEFAULT_ICON)->get());
      desktop->label->replace_token("%index%", to_string(n));
    }
  }

  /**
   * Generate module output
   */
  string xworkspaces_module::get_output() {
    string output;
    for (m_index = 0; m_index < m_viewports.size(); m_index++) {
      if (m_index > 0) {
        m_builder->space(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += static_module::get_output();
    }
    return output;
  }

  /**
   * Output content as defined in the config
   */
  bool xworkspaces_module::build(builder* builder, const string& tag) const {
    if (m_index >= m_viewports.size()) {
      m_log.warn("%s: Invalid index (index=%lu, viewports=%lu)", name(), m_index, m_viewports.size());
    } else if (tag == TAG_LABEL_MONITOR && m_viewports[m_index]->state != viewport_state::NONE) {
      builder->node(m_viewports[m_index]->label);
      return true;
    } else if (tag == TAG_LABEL_STATE) {
      size_t num{0};

      if (m_scroll) {
        builder->cmd(mousebtn::SCROLL_DOWN, string{EVENT_PREFIX} + string{EVENT_SCROLL_DOWN});
        builder->cmd(mousebtn::SCROLL_UP, string{EVENT_PREFIX} + string{EVENT_SCROLL_UP});
      }

      for (auto&& desktop : m_viewports[m_index]->desktops) {
        if (!desktop->label.get()) {
          continue;
        }

        if (m_click && desktop->state != desktop_state::ACTIVE) {
          builder->cmd(mousebtn::LEFT, string{EVENT_PREFIX} + string{EVENT_CLICK} + to_string(desktop->index));
          builder->node(desktop->label);
          builder->cmd_close();
        } else {
          builder->node(desktop->label);
        }

        num++;
      }

      if (m_scroll) {
        builder->cmd_close();
        builder->cmd_close();
      }

      return num > 0;
    }

    return false;
  }

  bool xworkspaces_module::input(string&& cmd) {
    size_t len{strlen(EVENT_PREFIX)};
    if (cmd.compare(0, len, EVENT_PREFIX) != 0) {
      return false;
    }
    cmd.erase(0, len);

    uint32_t new_desktop{0};
    uint32_t min_desktop{0};
    uint32_t max_desktop{0};
    uint32_t current_desktop{ewmh_util::get_current_desktop(m_ewmh.get())};

    for (auto&& viewport : m_viewports) {
      for (auto&& desktop : viewport->desktops) {
        if (min_desktop == 0) {
          min_desktop = desktop->index;
          max_desktop = math_util::max<uint16_t>(max_desktop, desktop->index);
        } else {
          min_desktop = math_util::min<uint16_t>(min_desktop, desktop->index);
          max_desktop = math_util::max<uint16_t>(max_desktop, desktop->index);
        }
      }
    }

    if ((len = strlen(EVENT_CLICK)) && cmd.compare(0, len, EVENT_CLICK) == 0) {
      new_desktop = std::strtoul(cmd.substr(len).c_str(), nullptr, 10);
    } else if ((len = strlen(EVENT_SCROLL_UP)) && cmd.compare(0, len, EVENT_SCROLL_UP) == 0) {
      new_desktop = math_util::min<uint32_t>(max_desktop, current_desktop + 1);
      if (new_desktop == current_desktop) {
        new_desktop = min_desktop;
      }
    } else if ((len = strlen(EVENT_SCROLL_DOWN)) && cmd.compare(0, len, EVENT_SCROLL_DOWN) == 0) {
      new_desktop = math_util::max<uint32_t>(min_desktop, std::max(0U, current_desktop - 1));
      if (new_desktop == current_desktop) {
        new_desktop = max_desktop;
      }
    }

    if (new_desktop != current_desktop) {
      m_log.info("%s: Requesting change to desktop #%u", name(), new_desktop);
      ewmh_util::change_current_desktop(m_ewmh.get(), new_desktop);
      m_connection.flush();
    } else {
      m_log.info("%s: Ignoring change to current desktop", name());
    }

    return true;
  }
}

POLYBAR_NS_END
