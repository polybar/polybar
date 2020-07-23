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
    m_formatter->add(
        DEFAULT_FORMAT, DEFAULT_FORMAT_TAGS, {TAG_LABEL_TAGS, TAG_LABEL_LAYOUT, TAG_LABEL_FLOATING, TAG_LABEL_TITLE});

    // Populate m_state_labels map with labels and their states
    if (m_formatter->has(TAG_LABEL_TAGS)) {
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
      m_layout_label = load_optional_label(m_conf, name(), "label-layout", "%symbol%");
    }

    if (m_formatter->has(TAG_LABEL_FLOATING)) {
      m_floating_label = load_optional_label(m_conf, name(), "label-floating", "");
    }

    if (m_formatter->has(TAG_LABEL_TITLE)) {
      m_title_label = load_optional_label(m_conf, name(), "label-title", "%title%");
    }

    m_tags_click = m_conf.get(name(), "enable-tags-click", m_tags_click);
    m_layout_click = m_conf.get(name(), "enable-layout-click", m_layout_click);
    m_layout_scroll = m_conf.get(name(), "enable-layout-scroll", m_layout_scroll);
    m_layout_wrap = m_conf.get(name(), "layout-scroll-wrap", m_layout_wrap);
    m_layout_reverse = m_conf.get(name(), "layout-scroll-reverse", m_layout_reverse);
    m_secondary_layout_symbol = m_conf.get(name(), "secondary-layout-symbol", m_secondary_layout_symbol);

    try {
      update_monitor_ref();
      m_focused_client_id = m_bar_mon->clients.selected;

      if (m_formatter->has(TAG_LABEL_TAGS)) {
        // Initialize tags array
        auto tags = m_ipc->get_tags();
        for (dwmipc::Tag& t : *tags) {
          auto state = get_state(t.bit_mask);
          auto label = m_state_labels.at(state)->clone();
          label->replace_token("%name%", t.tag_name);
          m_tags.emplace_back(t.tag_name, t.bit_mask, state, move(label));
        }

        // This event is for keeping track of the tag states
        m_ipc->on_tag_change = [this](const dwmipc::TagChangeEvent& ev) { this->on_tag_change(ev); };
        m_ipc->subscribe(dwmipc::Event::TAG_CHANGE);
      }

      if (m_floating_label) {
        update_floating_label();
      }

      if (m_layout_label) {
        auto layouts = m_ipc->get_layouts();
        m_layouts = m_ipc->get_layouts();

        // First layout is treated as default by dwm
        m_default_layout = &m_layouts->at(0);
        m_current_layout = find_layout(m_bar_mon->layout.address.cur);
        m_secondary_layout = find_layout(m_secondary_layout_symbol);

        if (m_secondary_layout == nullptr) {
          throw module_error("Secondary layout symbol does not exist");
        }

        // Initialize layout symbol
        update_layout_label(m_bar_mon->layout.symbol.cur);

        // This event is only needed to update the layout label
        m_ipc->on_layout_change = [this](const dwmipc::LayoutChangeEvent& ev) { on_layout_change(ev); };
        m_ipc->subscribe(dwmipc::Event::LAYOUT_CHANGE);
      }

      // These events are only necessary to update the focused window title
      if (m_title_label) {
        update_title_label();
        m_ipc->on_client_focus_change = [this](const dwmipc::ClientFocusChangeEvent& ev) {
          this->on_client_focus_change(ev);
        };
        m_ipc->subscribe(dwmipc::Event::CLIENT_FOCUS_CHANGE);

        m_ipc->on_focused_title_change = [this](const dwmipc::FocusedTitleChangeEvent& ev) {
          this->on_focused_title_change(ev);
        };
        m_ipc->subscribe(dwmipc::Event::FOCUSED_TITLE_CHANGE);
      }

      if (m_floating_label) {
        update_floating_label();
        m_ipc->on_focused_state_change = [this](const dwmipc::FocusedStateChangeEvent& ev) {
          this->on_focused_state_change(ev);
        };
        m_ipc->subscribe(dwmipc::Event::FOCUSED_STATE_CHANGE);
      }

      // This event is for keeping track of the currently focused monitor
      m_ipc->on_monitor_focus_change = [this](const dwmipc::MonitorFocusChangeEvent& ev) {
        this->on_monitor_focus_change(ev);
      };
      m_ipc->subscribe(dwmipc::Event::MONITOR_FOCUS_CHANGE);
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
      sleep(chrono::duration<double>(1));
      return reconnect_dwm();
    } catch (const dwmipc::IPCError& err) {
      m_log.err("%s: Failed to handle event (reason: %s)", name(), err.what());
    }
    return false;
  }

  auto dwm_module::update() -> bool {
    // All updates are handled in has_event
    return true;
  }

  auto dwm_module::build_cmd(const char* ipc_cmd, const string& arg) -> string {
    return EVENT_PREFIX + string(ipc_cmd) + "-" + arg;
  }

  auto dwm_module::build(builder* builder, const string& tag) const -> bool {
    if (tag == TAG_LABEL_TITLE) {
      builder->node(m_title_label);
    } else if (tag == TAG_LABEL_FLOATING) {
      if (!m_is_floating) {
        return true;
      }
      builder->node(m_floating_label);
    } else if (tag == TAG_LABEL_LAYOUT) {
      if (m_layout_click) {
        // Toggle between secondary and default layout
        auto addr = (m_current_layout == m_default_layout ? m_secondary_layout : m_default_layout)->address;
        builder->cmd(mousebtn::LEFT, build_cmd(CMD_LAYOUT_SET, to_string(addr)));
        // Set previous layout
        builder->cmd(mousebtn::RIGHT, build_cmd(CMD_LAYOUT_SET, "0"));
      }
      if (m_layout_scroll) {
        auto addr_next = next_layout(*m_current_layout, m_layout_wrap)->address;
        auto addr_prev = prev_layout(*m_current_layout, m_layout_wrap)->address;
        // Set address based on scroll direction
        auto scroll_down_addr = to_string(m_layout_reverse ? addr_prev : addr_next);
        auto scroll_up_addr = to_string(m_layout_reverse ? addr_next : addr_prev);
        builder->cmd(mousebtn::SCROLL_DOWN, build_cmd(CMD_LAYOUT_SET, scroll_down_addr));
        builder->cmd(mousebtn::SCROLL_UP, build_cmd(CMD_LAYOUT_SET, scroll_up_addr));
      }
      builder->node(m_layout_label);
      if (m_layout_click) {
        builder->cmd_close();
        builder->cmd_close();
      }
      if (m_layout_scroll) {
        builder->cmd_close();
        builder->cmd_close();
      }
    } else if (tag == TAG_LABEL_TAGS) {
      bool first = true;
      for (const auto& tag : m_tags) {
        // Don't insert separator before first tag
        if (first) {
          first = false;
        } else if (*m_seperator_label) {
          builder->node(m_seperator_label);
        }

        if (m_tags_click) {
          builder->cmd(mousebtn::LEFT, build_cmd(CMD_TAG_VIEW, to_string(tag.bit_mask)));
          builder->cmd(mousebtn::RIGHT, build_cmd(CMD_TAG_TOGGLE_VIEW, to_string(tag.bit_mask)));
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

  auto dwm_module::check_send_cmd(string cmd, const string& ipc_cmd) -> bool {
    // cmd = <EVENT_PREFIX><ipc_cmd>-<arg>
    cmd.erase(0, strlen(EVENT_PREFIX));

    // cmd = <ipc_cmd>-<arg>
    if (cmd.compare(0, ipc_cmd.size(), ipc_cmd) == 0) {
      // Erase '<ipc_cmd>-'
      cmd.erase(0, ipc_cmd.size() + 1);
      m_log.info("%s: Sending workspace %s command to ipc handler", name(), ipc_cmd);

      try {
        m_ipc->run_command(ipc_cmd, stoul(cmd));
        return true;
      } catch (const dwmipc::SocketClosedError& err) {
        m_log.err("%s: Disconnected from socket: %s", name(), err.what());
        sleep(chrono::duration<double>(1));
        reconnect_dwm();
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

    return check_send_cmd(cmd, CMD_TAG_VIEW) || check_send_cmd(cmd, CMD_TAG_TOGGLE_VIEW) ||
           check_send_cmd(cmd, CMD_LAYOUT_SET);
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

  auto dwm_module::find_layout(const string& sym) const -> const dwmipc::Layout* {
    for (const auto& lt : *m_layouts) {
      if (lt.symbol == sym) {
        return &lt;
      }
    }
    return nullptr;
  }

  auto dwm_module::find_layout(const uintptr_t addr) const -> const dwmipc::Layout* {
    for (const auto& lt : *m_layouts) {
      if (lt.address == addr) {
        return &lt;
      }
    }
    return nullptr;
  }

  auto dwm_module::next_layout(const dwmipc::Layout& layout, bool wrap) const -> const dwmipc::Layout* {
    const auto* next = &layout + 1;
    const auto* first = m_layouts->begin().base();
    if (next < m_layouts->end().base()) {
      return next;
    } else if (wrap) {
      return first;
    } else {
      return &layout;
    }
  }

  auto dwm_module::prev_layout(const dwmipc::Layout& layout, bool wrap) const -> const dwmipc::Layout* {
    const auto* prev = &layout - 1;
    const auto* last = m_layouts->end().base() - 1;
    if (prev >= m_layouts->begin().base()) {
      return prev;
    } else if (wrap) {
      return last;
    } else {
      return &layout;
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

  void dwm_module::update_title_label(const string& title) {
    m_title_label->reset_tokens();
    m_title_label->replace_token("%title%", title);
  }

  void dwm_module::update_title_label() {
    std::string new_title;
    if (m_focused_client_id != 0) {
      try {
        new_title = m_ipc->get_client(m_focused_client_id)->name;
      } catch (const dwmipc::SocketClosedError& err) {
        m_log.err("%s: Disconnected from socket: %s", name(), err.what());
        sleep(chrono::duration<double>(1));
        reconnect_dwm();
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    }
    update_title_label(new_title);
  }

  void dwm_module::update_floating_label() {
    if (m_focused_client_id != 0) {
      try {
        m_is_floating = m_ipc->get_client(m_focused_client_id)->states.is_floating;
      } catch (const dwmipc::SocketClosedError& err) {
        m_log.err("%s: Disconnected from socket: %s", name(), err.what());
        sleep(chrono::duration<double>(1));
        reconnect_dwm();
      } catch (const dwmipc::IPCError& err) {
        throw module_error(err.what());
      }
    } else {
      m_is_floating = false;
    }
  }

  void dwm_module::update_layout_label(const string& symbol) {
    m_layout_label->reset_tokens();
    m_layout_label->replace_token("%symbol%", symbol);
  }

  auto dwm_module::reconnect_dwm() -> bool {
    try {
      if (!m_ipc->is_main_socket_connected()) {
        m_log.notice("%s: Attempting to reconnect to main socket", name());
        m_ipc->connect_main_socket();
        m_log.notice("%s: Successfully reconnected to main socket", name());
      }
      if (!m_ipc->is_event_socket_connected()) {
        m_log.notice("%s: Attempting to reconnect event socket", name());
        m_ipc->connect_event_socket();
        m_log.notice("%s: Successfully reconnected event to socket", name());
      }
      return true;
    } catch (const dwmipc::IPCError& err) {
      m_log.err("%s: Failed to reconnect to socket: %s", name(), err.what());
    }
    return false;
  }

  void dwm_module::on_layout_change(const dwmipc::LayoutChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num) {
      m_current_layout = find_layout(ev.new_address);
      update_layout_label(ev.new_symbol);
    }
  }

  void dwm_module::on_monitor_focus_change(const dwmipc::MonitorFocusChangeEvent& ev) {
    m_active_mon = &m_monitors->at(ev.new_mon_num);
    if (m_formatter->has(TAG_LABEL_TAGS)) {
      update_tag_labels();
    }
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
      update_title_label(ev.new_name);
    }
  }

  void dwm_module::on_focused_state_change(const dwmipc::FocusedStateChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num && ev.client_window_id == m_focused_client_id) {
      m_is_floating = ev.new_state.is_floating;
    }
  }

  void dwm_module::on_client_focus_change(const dwmipc::ClientFocusChangeEvent& ev) {
    if (ev.monitor_num == m_bar_mon->num) {
      m_focused_client_id = ev.new_win_id;
      update_title_label();
      update_floating_label();
    }
  }

}  // namespace modules

POLYBAR_NS_END
