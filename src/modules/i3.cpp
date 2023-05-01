#include "modules/i3.hpp"

#include <sys/socket.h>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/file.hpp"

#include <i3ipc++/ipc-util.hpp>

POLYBAR_NS

namespace modules {
  template class module<i3_module>;

  i3_module::i3_module(const bar_settings& bar, string name_, const config& conf)
      : event_module<i3_module>(bar, move(name_), conf) {
    m_router->register_action_with_data(EVENT_FOCUS, [this](const std::string& data) { action_focus(data); });
    m_router->register_action(EVENT_NEXT, [this]() { action_next(); });
    m_router->register_action(EVENT_PREV, [this]() { action_prev(); });

    try {
      auto socket_path = i3ipc::get_socketpath();
      if (!file_util::exists(socket_path)) {
        throw module_error("i3 socket does not exist: " + (socket_path.empty() ? "<empty>" : socket_path));
      } else {
        m_log.info("%s: Found i3 socket at '%s'", name(), socket_path);
      }
    } catch (const i3ipc::ipc_error& e) {
      throw module_error("Could not find i3 socket: " + string(e.what()));
    }

    m_ipc = std::make_unique<i3ipc::connection>();

    // Load configuration values
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);
    m_revscroll = m_conf.get(name(), "reverse-scroll", m_revscroll);
    m_wrap = m_conf.get(name(), "wrapping-scroll", m_wrap);
    m_indexsort = m_conf.get(name(), "index-sort", m_indexsort);
    m_pinworkspaces = m_conf.get(name(), "pin-workspaces", m_pinworkspaces);
    m_show_urgent = m_conf.get(name(), "show-urgent", m_show_urgent);
    m_strip_wsnumbers = m_conf.get(name(), "strip-wsnumbers", m_strip_wsnumbers);
    m_fuzzy_match = m_conf.get(name(), "fuzzy-match", m_fuzzy_match);

    m_conf.warn_deprecated(name(), "wsname-maxlen", "%name:min:max%");

    // Add formats and create components
    m_formatter->add(DEFAULT_FORMAT, DEFAULT_TAGS, {TAG_LABEL_STATE, TAG_LABEL_MODE});

    if (m_formatter->has(TAG_LABEL_STATE)) {
      m_statelabels.insert(
          make_pair(state::FOCUSED, load_optional_label(m_conf, name(), "label-focused", DEFAULT_WS_LABEL)));
      m_statelabels.insert(
          make_pair(state::UNFOCUSED, load_optional_label(m_conf, name(), "label-unfocused", DEFAULT_WS_LABEL)));
      m_statelabels.insert(
          make_pair(state::VISIBLE, load_optional_label(m_conf, name(), "label-visible", DEFAULT_WS_LABEL)));
      m_statelabels.insert(
          make_pair(state::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_WS_LABEL)));
    }

    if (m_formatter->has(TAG_LABEL_MODE)) {
      m_modelabel = load_optional_label(m_conf, name(), "label-mode", "%mode%");
    }

    m_labelseparator = load_optional_label(m_conf, name(), "label-separator", "");

    m_icons = std::make_shared<iconset>();
    m_icons->add(DEFAULT_WS_ICON, std::make_shared<label>(m_conf.get(name(), DEFAULT_WS_ICON, ""s)));

    int i = 0;
    for (const auto& workspace : m_conf.get_list<string>(name(), "ws-icon", {})) {
      auto vec = string_util::tokenize(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], std::make_shared<label>(vec[1]));
      } else {
        m_log.err("%s: Ignoring ws-icon-%d because it has %s semicolons", name(), i, vec.size() > 2? "too many" : "too few");
      }

      i++;
    }

    try {
      if (m_modelabel) {
        m_ipc->on_mode_event = [this](const i3ipc::mode_t& mode) {
          m_modeactive = (mode.change != DEFAULT_MODE);
          if (m_modeactive) {
            m_modelabel->reset_tokens();
            m_modelabel->replace_token("%mode%", mode.change);
          }
        };
      }
      m_ipc->subscribe(i3ipc::ET_WORKSPACE | i3ipc::ET_MODE);
    } catch (const exception& err) {
      throw module_error(err.what());
    }
  }

  i3_module::workspace::operator bool() {
    return label && *label;
  }

  void i3_module::stop() {
    try {
      if (m_ipc) {
        m_log.info("%s: Disconnecting from socket", name());
        shutdown(m_ipc->get_event_socket_fd(), SHUT_RDWR);
        shutdown(m_ipc->get_main_socket_fd(), SHUT_RDWR);
      }
    } catch (...) {
    }

    event_module::stop();
  }

  bool i3_module::has_event() {
    try {
      m_ipc->handle_event();
      return true;
    } catch (const exception& err) {
      try {
        m_log.warn("%s: Attempting to reconnect socket (reason: %s)", name(), err.what());
        m_ipc->connect_event_socket(true);
        m_log.info("%s: Reconnecting socket succeeded", name());
      } catch (const exception& err) {
        m_log.err("%s: Failed to reconnect socket (reason: %s)", name(), err.what());
      }
      return false;
    }
  }

  bool i3_module::update() {
    /*
     * update only populates m_workspaces and those are only needed when
     * <label-state> appears in the format
     */
    if (!m_formatter->has(TAG_LABEL_STATE)) {
      return true;
    }
    m_workspaces.clear();
    i3_util::connection_t ipc;

    try {
      vector<shared_ptr<i3_util::workspace_t>> workspaces;

      if (m_pinworkspaces) {
        workspaces = i3_util::workspaces(ipc, m_bar.monitor->name, m_show_urgent);
      } else {
        workspaces = i3_util::workspaces(ipc);
      }

      if (m_indexsort) {
        sort(workspaces.begin(), workspaces.end(), i3_util::ws_numsort);
      }

      for (auto&& ws : workspaces) {
        state ws_state{state::NONE};

        if (ws->focused) {
          ws_state = state::FOCUSED;
        } else if (ws->urgent) {
          ws_state = state::URGENT;
        } else if (ws->visible) {
          ws_state = state::VISIBLE;
        } else {
          ws_state = state::UNFOCUSED;
        }

        string ws_name{ws->name};

        // Remove workspace numbers "0:"
        if (m_strip_wsnumbers) {
          ws_name.erase(0, string_util::find_nth(ws_name, 0, ":", 1) + 1);
        }

        // Trim leading and trailing whitespace
        ws_name = string_util::trim(move(ws_name), ' ');

        auto icon = m_icons->get(ws->name, DEFAULT_WS_ICON, m_fuzzy_match);
        auto label = m_statelabels.find(ws_state)->second->clone();

        label->reset_tokens();
        label->replace_token("%output%", ws->output);
        label->replace_token("%name%", ws_name);
        label->replace_token("%icon%", icon->get());
        label->replace_token("%index%", to_string(ws->num));
        m_workspaces.emplace_back(std::make_unique<workspace>(ws->name, ws_state, move(label)));
      }

      return true;
    } catch (const exception& err) {
      m_log.err("%s: %s", name(), err.what());
      return false;
    }
  }

  bool i3_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_MODE && m_modeactive) {
      builder->node(m_modelabel);
    } else if (tag == TAG_LABEL_STATE && !m_workspaces.empty()) {
      if (m_scroll) {
        builder->action(mousebtn::SCROLL_DOWN, *this, m_revscroll ? EVENT_NEXT : EVENT_PREV, "");
        builder->action(mousebtn::SCROLL_UP, *this, m_revscroll ? EVENT_PREV : EVENT_NEXT, "");
      }

      bool first = true;
      for (auto&& ws : m_workspaces) {
        /*
         * The separator should only be inserted in between the workspaces, so
         * we insert it in front of all workspaces except the first one.
         */
        if (first) {
          first = false;
        } else if (*m_labelseparator) {
          builder->node(m_labelseparator);
        }

        if (m_click) {
          builder->action(mousebtn::LEFT, *this, EVENT_FOCUS, ws->name, ws->label);
        } else {
          builder->node(ws->label);
        }
      }

      if (m_scroll) {
        builder->action_close();
        builder->action_close();
      }
    } else {
      return false;
    }

    return true;
  }

  void i3_module::action_focus(const string& ws) {
    const i3_util::connection_t conn{};
    m_log.info("%s: Sending workspace focus command to ipc handler", name());
    conn.send_command(make_workspace_command(ws));
  }

  void i3_module::action_next() {
    focus_direction(true);
  }

  void i3_module::action_prev() {
    focus_direction(false);
  }

  void i3_module::focus_direction(bool next) {
    const i3_util::connection_t conn{};

    auto workspaces = i3_util::workspaces(conn, m_bar.monitor->name);
    auto current_ws = std::find_if(workspaces.begin(), workspaces.end(), [](auto ws) { return ws->visible; });

    if (current_ws == workspaces.end()) {
      m_log.warn("%s: Current workspace not found", name());
      return;
    }

    if (next && (m_wrap || std::next(current_ws) != workspaces.end())) {
      if (!(*current_ws)->focused) {
        m_log.info("%s: Sending workspace focus command to ipc handler", name());
        conn.send_command(make_workspace_command((*current_ws)->name));
      }
      m_log.info("%s: Sending workspace next_on_output command to ipc handler", name());
      conn.send_command("workspace next_on_output");
    } else if (!next && (m_wrap || current_ws != workspaces.begin())) {
      if (!(*current_ws)->focused) {
        m_log.info("%s: Sending workspace focus command to ipc handler", name());
        conn.send_command(make_workspace_command((*current_ws)->name));
      }
      m_log.info("%s: Sending workspace prev_on_output command to ipc handler", name());
      conn.send_command("workspace prev_on_output");
    }
  }

  string i3_module::make_workspace_command(const string& workspace) {
    return "workspace \"" + workspace + "\"";
  }
}  // namespace modules

POLYBAR_NS_END
