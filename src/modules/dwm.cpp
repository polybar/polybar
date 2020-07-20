#include "modules/dwm.hpp"

#include <dwmipcpp/errors.hpp>

#include "components/builder.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/env.hpp"
#include "utils/file.hpp"

POLYBAR_NS

namespace modules {
  template class module<dwm_module>;

  dwm_module::dwm_module(const bar_settings& bar, string name_) : event_module<dwm_module>(bar, move(name_)) {
    string socket_path = env_util::get("DWM_SOCKET");
    if (socket_path.empty()) {
      // Defined by cmake
      socket_path = DWM_SOCKET_PATH;
    }

    if (!file_util::exists(socket_path)) {
      throw module_error("Could not find socket: " + (socket_path.empty() ? "<empty>" : socket_path));
    }

    m_ipc = factory_util::unique<dwmipc::Connection>(socket_path);

    // Load configuration
    m_formatter->add(DEFAULT_FORMAT, DEFAULT_FORMAT_TAGS, {TAG_LABEL_STATE, TAG_LABEL_LAYOUT, TAG_LABEL_TITLE});

    // Populate m_state_labels map with labels and their states
    if (m_formatter->has(TAG_LABEL_STATE)) {
      m_state_labels.insert(
          std::make_pair(state_t::FOCUSED, load_optional_label(m_conf, name(), "label-focused", DEFAULT_STATE_LABEL)));
      m_state_labels.insert(std::make_pair(
          state_t::UNFOCUSED, load_optional_label(m_conf, name(), "label-unfocused", DEFAULT_STATE_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::VISIBLE, load_optional_label(m_conf, name(), "label-visible", DEFAULT_STATE_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_STATE_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::EMPTY, load_optional_label(m_conf, name(), "label-empty", DEFAULT_STATE_LABEL)));
    }

    m_seperator_label = load_optional_label(m_conf, name(), "label-separator", "");

    if (m_formatter->has(TAG_LABEL_LAYOUT)) {
      m_layout_label = load_optional_label(m_conf, name(), "label-layout", "%layout%");
    }

    if (m_formatter->has(TAG_LABEL_TITLE)) {
      m_title_label = load_optional_label(m_conf, name(), "label-title", "%title%");
    }

    m_click = m_conf.get(name(), "enable-click", m_click);

    try {
      update_monitor_ref();

      // Initialize tags array
      auto tags = m_ipc->get_tags();
      for (dwmipc::Tag& t : *tags) {
        auto state = get_state(t.bit_mask);
        auto label = m_state_labels.at(state)->clone();
        label->replace_token("%name%", t.tag_name);
        m_tags.emplace_back(t.tag_name, t.bit_mask, state, move(label));
      }

      // This event is only needed to update the layout label
      if (m_layout_label) {
        auto layouts = m_ipc->get_layouts();
        // Initialize layout symbol
        m_layout_label->replace_token("%layout%", m_bar_mon->layout.symbol.cur);
        m_ipc->on_layout_change = [this](const dwmipc::LayoutChangeEvent& ev) { on_layout_change(ev); };
        m_ipc->subscribe(dwmipc::Event::LAYOUT_CHANGE);
      }

      // These events are only necessary to update the focused window title
      if (m_title_label) {
        update_title_label(m_bar_mon->clients.selected);
        m_ipc->on_client_focus_change = [this](const dwmipc::ClientFocusChangeEvent& ev) {
          this->on_client_focus_change(ev);
        };
        m_ipc->subscribe(dwmipc::Event::CLIENT_FOCUS_CHANGE);

        m_ipc->on_focused_title_change = [this](const dwmipc::FocusedTitleChangeEvent& ev) {
          this->on_focused_title_change(ev);
        };
        m_ipc->subscribe(dwmipc::Event::FOCUSED_TITLE_CHANGE);
      }

      // This event is for keeping track of the currently focused monitor
      m_ipc->on_monitor_focus_change = [this](const dwmipc::MonitorFocusChangeEvent& ev) {
        this->on_monitor_focus_change(ev);
      };
      m_ipc->subscribe(dwmipc::Event::MONITOR_FOCUS_CHANGE);

      // This event is for keeping track of the tag states
      m_ipc->on_tag_change = [this](const dwmipc::TagChangeEvent& ev) { this->on_tag_change(ev); };
      m_ipc->subscribe(dwmipc::Event::TAG_CHANGE);
    } catch (const dwmipc::IPCError& err) {
      throw module_error(err.what());
    }
  }

  void dwm_module::stop() {
    try {
      m_log.info("%s: Disconnecting from socket", name());
      m_ipc.reset();
    } catch (...) {
    }

    event_module::stop();
  }

  auto dwm_module::has_event() -> bool {
    try {
      return m_ipc->handle_event();
    } catch (const dwmipc::SocketClosedError& err) {
      m_log.err("%s: Disconnected from socket: %s", name(), err.what());
      return reconnect_dwm();
    } catch (const exception& err) {
      m_log.err("%s: Failed to handle event (reason: %s)", name(), err.what());
    }
    return false;
  }

  auto dwm_module::update() -> bool {
    // All updates are handled in has_event
    return true;
  }

  auto dwm_module::build(builder* builder, const string& tag) const -> bool {
    if (tag == TAG_LABEL_LAYOUT) {
      builder->node(m_layout_label);
    } else if (tag == TAG_LABEL_TITLE) {
      builder->node(m_title_label);
    } else if (tag == TAG_LABEL_STATE) {
      bool first = true;
      for (const auto& tag : m_tags) {
        // Don't insert separator before first tag
        if (first) {
          first = false;
        } else if (*m_seperator_label) {
          builder->node(m_seperator_label);
        }

        if (m_click) {
          builder->cmd(mousebtn::LEFT, EVENT_PREFIX + string{EVENT_LCLICK} + "-" + to_string(tag.bit_mask));
          builder->cmd(mousebtn::RIGHT, EVENT_PREFIX + string{EVENT_RCLICK} + "-" + to_string(tag.bit_mask));
          builder->node(tag.label);
          builder->cmd_close();
          builder->cmd_close();
        } else {
          builder->node(tag.label);
        }
      }
    } else {
      return false;
    }
    return true;
  }

  auto dwm_module::check_send_cmd(string cmd, const string& ev_name) -> bool {
    std::cerr << cmd << std::endl;
    // cmd = <EVENT_PREFIX><ev_name>-<arg>
    cmd.erase(0, strlen(EVENT_PREFIX));

    // cmd = <ev_name>-<arg>
    if (cmd.compare(0, ev_name.size(), ev_name) == 0) {
      // Erase '<ev_name>-'
      cmd.erase(0, ev_name.size() + 1);
      m_log.info("%s: Sending workspace %s command to ipc handler", ev_name, name());

      try {
        m_ipc->run_command(ev_name, stoul(cmd));
        return true;
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    }
    return false;
  }

  auto dwm_module::input(string&& cmd) -> bool {
    if (cmd.find(EVENT_PREFIX) != 0) {
      return false;
    }

    return check_send_cmd(cmd, EVENT_LCLICK) || check_send_cmd(cmd, EVENT_RCLICK);
  }

  auto dwm_module::get_state(tag_mask_t bit_mask) const -> state_t {
    // Tag selected > occupied > urgent
    // Monitor selected - Tag selected FOCUSED
    // Monitor unselected - Tag selected UNFOCUSED
    // Tag unselected - Tag occupied - Tag non-urgent VISIBLE
    // Tag unselected - Tag occupied - Tag urgent URGENT
    // Tag unselected - Tag unoccupied EMPTY

    auto tag_state = m_bar_mon->tag_state;
    bool is_mon_active = m_bar_mon == m_active_mon;

    if (is_mon_active && tag_state.selected & bit_mask) {
      // Tag selected on selected monitor
      return state_t::FOCUSED;
    } else if (tag_state.urgent & bit_mask) {
      // Tag is urgent
      return state_t::URGENT;
    } else if (!is_mon_active && tag_state.selected & bit_mask) {
      // Tag is selected, but not on selected monitor
      return state_t::UNFOCUSED;
    } else if (tag_state.occupied & bit_mask) {
      // Tag is occupied, but not selected
      return state_t::VISIBLE;
    }

    return state_t::EMPTY;
  }

  void dwm_module::update_monitor_ref() {
    m_monitors = m_ipc->get_monitors();
    // Sort by increasing monitor index
    std::sort(m_monitors->begin(), m_monitors->end(),
        [](dwmipc::Monitor& m1, dwmipc::Monitor& m2) { return m1.num < m2.num; });

    for (const dwmipc::Monitor& m : *m_monitors) {
      const auto& geom = m.monitor_geom;
      const auto& bmon = *m_bar.monitor;
      // Compare geometry
      if (geom.x == bmon.x && geom.y == bmon.y && geom.width == bmon.w && geom.height == bmon.h) {
        m_bar_mon = &m;
      }

      if (m.is_selected) {
        m_active_mon = &m;
      }
    }
  }

  void dwm_module::update_tag_labels() {
    for (auto& t : m_tags) {
      t.state = get_state(t.bit_mask);
      t.label = m_state_labels.at(t.state)->clone();
      t.label->reset_tokens();
      t.label->replace_token("%name%", t.name);
    }
  }

  void dwm_module::update_title_label(unsigned int client_id) {
    std::string new_title;
    if (client_id != 0) {
      try {
        new_title = m_ipc->get_client(client_id)->name;
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    }
    m_title_label->reset_tokens();
    m_title_label->replace_token("%title%", new_title);
  }

  auto dwm_module::reconnect_dwm() -> bool {
    try {
      if (!m_ipc->is_main_socket_connected()) {
        m_log.info("%s: Attempting to reconnect to main socket", name());
        m_ipc->connect_main_socket();
        m_log.info("%s: Successfully reconnected to main socket", name());
      }
      if (!m_ipc->is_event_socket_connected()) {
        m_log.info("%s: Attempting to reconnect event socket", name());
        m_ipc->connect_event_socket();
        m_log.info("%s: Successfully reconnected event to socket", name());
      }
      return true;
    } catch (const exception& err) {
      m_log.err("%s: Failed to reconnect to socket: %s", name(), err.what());
    }
    return false;
  }

  void dwm_module::on_layout_change(const dwmipc::LayoutChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num) {
      m_layout_label->reset_tokens();
      m_layout_label->replace_token("%layout%", ev.new_symbol);
    }
  }

  void dwm_module::on_monitor_focus_change(const dwmipc::MonitorFocusChangeEvent& ev) {
    m_active_mon = &m_monitors->at(ev.new_mon_num);
    update_tag_labels();
  }

  void dwm_module::on_tag_change(const dwmipc::TagChangeEvent& ev) {
    auto& mon = m_monitors->at(ev.monitor_num);
    mon.tag_state = ev.new_state;

    if (ev.monitor_num == m_bar_mon->num) {
      update_tag_labels();
    }
  }

  void dwm_module::on_focused_title_change(const dwmipc::FocusedTitleChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num && ev.client_window_id == m_focused_client_id) {
      m_title_label->reset_tokens();
      m_title_label->replace_token("%title%", ev.new_name);
    }
  }

  void dwm_module::on_client_focus_change(const dwmipc::ClientFocusChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num) {
      m_focused_client_id = ev.new_win_id;
      update_title_label(ev.new_win_id);
    }
  }

}  // namespace modules

POLYBAR_NS_END
