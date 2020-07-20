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
      socket_path = DWM_SOCKET_PATH;
    }

    if (!file_util::exists(socket_path)) {
      throw module_error("Could not find socket: " + (socket_path.empty() ? "<empty>" : socket_path));
    }

    m_ipc = factory_util::unique<dwmipc::Connection>(socket_path);

    // Load configuration
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_pin_tags = m_conf.get(name(), "pin-tags", m_pin_tags);

    m_formatter->add(DEFAULT_FORMAT, DEFAULT_FORMAT_TAGS, {TAG_LABEL_STATE, TAG_LABEL_LAYOUT, TAG_LABEL_TITLE});

    if (m_formatter->has(TAG_LABEL_STATE)) {
      m_state_labels.insert(
          std::make_pair(state_t::FOCUSED, load_optional_label(m_conf, name(), "label-focused", DEFAULT_TAG_LABEL)));
      m_state_labels.insert(std::make_pair(
          state_t::UNFOCUSED, load_optional_label(m_conf, name(), "label-unfocused", DEFAULT_TAG_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::VISIBLE, load_optional_label(m_conf, name(), "label-visible", DEFAULT_TAG_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_TAG_LABEL)));
      m_state_labels.insert(
          std::make_pair(state_t::NONE, load_optional_label(m_conf, name(), "label-none", DEFAULT_TAG_LABEL)));
    }

    if (m_formatter->has(TAG_LABEL_LAYOUT)) {
      m_layout_label = load_optional_label(m_conf, name(), "label-layout", "%layout%");
    }

    if (m_formatter->has(TAG_LABEL_TITLE)) {
      m_title_label = load_optional_label(m_conf, name(), "label-title", "%title%");
      m_title_label->replace_token("%title%", "");
    }

    m_seperator_label = load_optional_label(m_conf, name(), "label-separator", "");

    try {
      m_monitors = m_ipc->get_monitors();
      std::sort(m_monitors->begin(), m_monitors->end(),
          [](dwmipc::Monitor& m1, dwmipc::Monitor& m2) { return m1.num < m2.num; });
      update_monitor_ref();

      auto selected_client = m_monitors->at(m_bar_mon).clients.selected;
      std::shared_ptr<dwmipc::Client> c;
      if (selected_client != 0) {
        c = m_ipc->get_client(selected_client);
      }

      m_title_label->reset_tokens();
      m_title_label->replace_token("%title%", c->name);

      auto tags = m_ipc->get_tags();
      // m_tags.resize(tags->size());

      for (dwmipc::Tag& t : *tags) {
        auto state = get_state(t.bit_mask);
        auto label = m_state_labels.at(state)->clone();
        label->replace_token("%name%", t.tag_name);
        m_tags.emplace_back(t.tag_name, t.bit_mask, state, move(label));
      }

      if (m_layout_label) {
        auto layouts = m_ipc->get_layouts();
        m_layout_label->replace_token("%layout%", m_monitors->at(m_bar_mon).layout.symbol.cur);
        m_ipc->on_layout_change = [this](const dwmipc::LayoutChangeEvent& ev) {
          m_layout_label->reset_tokens();
          m_layout_label->replace_token("%layout%", ev.new_symbol);
        };
      }

      m_ipc->on_monitor_focus_change = [this](const dwmipc::MonitorFocusChangeEvent& ev) {
        m_active_mon_num = ev.new_mon_num;

        for (auto& t : m_tags) {
          t.state = get_state(t.bit_mask);
          t.label = m_state_labels.at(t.state)->clone();
          t.label->reset_tokens();
          t.label->replace_token("%name%", t.name);
        }
      };

      m_ipc->on_tag_change = [this](const dwmipc::TagChangeEvent& ev) {
        auto& mon = m_monitors->at(ev.monitor_num);
        mon.tag_state = ev.new_state;

        if (ev.monitor_num == m_bar_mon) {
          for (auto& t : m_tags) {
            t.state = get_state(t.bit_mask);
            t.label = m_state_labels.at(t.state)->clone();
            t.label->reset_tokens();
            t.label->replace_token("%name%", t.name);
          }
        }
      };

      m_ipc->on_client_focus_change = [this](const dwmipc::ClientFocusChangeEvent& ev) {
        if (ev.monitor_num != m_bar_mon) {
          return;
        }

        m_focused_client_id = ev.new_win_id;
        m_title_label->reset_tokens();
        if (m_focused_client_id != 0) {
          auto focused_client = m_ipc->get_client(m_focused_client_id);
          m_title_label->replace_token("%title%", focused_client->name);
        } else {
          m_title_label->replace_token("%title%", "");
        }
      };

      m_ipc->on_focused_title_change = [this](const dwmipc::FocusedTitleChangeEvent& ev) {
        if (ev.monitor_num == m_bar_mon && ev.client_window_id == m_focused_client_id) {
          m_title_label->reset_tokens();
          m_title_label->replace_token("%title%", ev.new_name);
        }
      };

      m_ipc->subscribe(dwmipc::Event::LAYOUT_CHANGE);
      m_ipc->subscribe(dwmipc::Event::CLIENT_FOCUS_CHANGE);
      m_ipc->subscribe(dwmipc::Event::TAG_CHANGE);
      m_ipc->subscribe(dwmipc::Event::MONITOR_FOCUS_CHANGE);
      m_ipc->subscribe(dwmipc::Event::FOCUSED_TITLE_CHANGE);
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

  bool dwm_module::has_event() {
    try {
      return m_ipc->handle_event();
    } catch (const dwmipc::SocketClosedError& err) {
      m_log.err("%s: Disconnected from socket: %s", name(), err.what());
      try {
        if (!m_ipc->is_main_socket_connected()) {
          m_log.info("%s: Attempting to reconnect to main socket", name());
          m_ipc->connect_main_socket();
          m_log.info("%s: Successfully reconnected to main socket", name());
        } else if (!m_ipc->is_event_socket_connected()) {
          m_log.info("%s: Attempting to reconnect event socket", name());
          m_ipc->connect_event_socket();
          m_log.info("%s: Successfully reconnected event to socket", name());
        }
      } catch (const exception& err) {
        m_log.err("%s: Failed to reconnect to socket: %s", name(), err.what());
      }
    } catch (const exception& err) {
      m_log.err("%s: Failed to handle event (reason: %s)", name(), err.what());
    }
    return false;
  }

  bool dwm_module::update() {
    return true;
  }

  bool dwm_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_LAYOUT) {
      builder->node(m_layout_label);
    } else if (tag == TAG_LABEL_TITLE) {
      builder->node(m_title_label);
    } else if (tag == TAG_LABEL_STATE) {
      bool first = true;
      for (const auto& tag : m_tags) {
        if ((tag.state == state_t::NONE && m_pin_tags) || (!m_pin_tags && tag.state != state_t::NONE)) {
          if (first) {
            first = false;
          } else if (*m_seperator_label) {
            builder->node(m_seperator_label);
          }

          if (m_click) {
            builder->cmd(mousebtn::LEFT, string{EVENT_LCLICK} + to_string(tag.bit_mask));
            builder->cmd(mousebtn::RIGHT, string{EVENT_RCLICK} + to_string(tag.bit_mask));
            builder->node(tag.label);
            builder->cmd_close();
            builder->cmd_close();
          } else {
            builder->node(tag.label);
          }
        }
      }
    } else {
      return false;
    }
    return true;
  }

  bool dwm_module::input(string&& cmd) {
    if (cmd.find(EVENT_PREFIX) != 0) {
      return false;
    }

    if (cmd.compare(0, strlen(EVENT_LCLICK), EVENT_LCLICK) == 0) {
      cmd.erase(0, strlen(EVENT_LCLICK));
      m_log.info("%s: Sending workspace view command to ipc handler", name());

      try {
        m_ipc->run_command("view", stoul(cmd));
        return true;
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    } else if (cmd.compare(0, strlen(EVENT_RCLICK), EVENT_RCLICK) == 0) {
      cmd.erase(0, strlen(EVENT_RCLICK));
      m_log.info("%s: Sending workspace toggleview command to ipc handler", name());

      try {
        m_ipc->run_command("toggleview", stoul(cmd));
        return true;
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    }
    return true;
  }

  dwm_module::state_t dwm_module::get_state(tag_mask_t bit_mask) const {
    // Tag selected > occupied > urgent
    // Monitor selected - Tag selected FOCUSED
    // Monitor unselected - Tag selected UNFOCUSED
    // Tag unselected - Tag occupied - Tag non-urgent VISIBLE
    // Tag unselected - Tag occupied - Tag urgent URGENT
    // Tag unselected - Tag unoccupied PIN?

    auto tag_state = m_monitors->at(m_bar_mon).tag_state;
    bool is_mon_active = m_bar_mon == m_active_mon_num;

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

    return state_t::NONE;
  }

  void dwm_module::update_monitor_ref() {
    for (const dwmipc::Monitor& m : *m_monitors) {
      const auto& geom = m.monitor_geom;
      const auto& bmon = *m_bar.monitor;
      if (geom.x == bmon.x && geom.y == bmon.y && geom.width == bmon.w && geom.height == bmon.h) {
        m_bar_mon = m.num;
      }

      if (m.is_selected) {
        m_active_mon_num = m.num;
      }
    }
  }

}  // namespace modules

POLYBAR_NS_END
