#include <sys/socket.h>

#include "modules/i3.hpp"

LEMONBUDDY_NS

namespace modules {
  void i3_module::setup() {
    // Load configuration values {{{

    GET_CONFIG_VALUE(name(), m_indexsort, "index-sort");
    GET_CONFIG_VALUE(name(), m_pinworkspaces, "pin-workspaces");
    GET_CONFIG_VALUE(name(), m_strip_wsnumbers, "strip-wsnumbers");
    GET_CONFIG_VALUE(name(), m_wsname_maxlen, "wsname-maxlen");

    // }}}
    // Add formats and create components {{{

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE});

    if (m_formatter->has(TAG_LABEL_STATE)) {
      m_statelabels.insert(make_pair(i3_flag::WORKSPACE_FOCUSED,
          load_optional_label(m_conf, name(), "label-focused", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(i3_flag::WORKSPACE_UNFOCUSED,
          load_optional_label(m_conf, name(), "label-unfocused", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(i3_flag::WORKSPACE_VISIBLE,
          load_optional_label(m_conf, name(), "label-visible", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(i3_flag::WORKSPACE_URGENT,
          load_optional_label(m_conf, name(), "label-urgent", DEFAULT_WS_LABEL)));
    }

    m_icons = iconset_t{new iconset()};
    m_icons->add(
        DEFAULT_WS_ICON, icon_t{new icon(m_conf.get<string>(name(), DEFAULT_WS_ICON, ""))});

    // }}}
    // Add formats and create components {{{

    for (auto workspace : m_conf.get_list<string>(name(), "ws-icon", {})) {
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2)
        m_icons->add(vec[0], icon_t{new icon{vec[1]}});
    }

    // }}}
    // Subscribe to ipc events {{{

    try {
      m_ipc.subscribe(i3ipc::ET_WORKSPACE);
      m_ipc.prepare_to_event_handling();
    } catch (std::runtime_error& err) {
      throw module_error(err.what());
    } catch (std::exception& err) {
      throw module_error(err.what());
    }

    // }}}
  }

  void i3_module::stop() {
    // Shutdown ipc connection when stopping the module {{{

    try {
      shutdown(m_ipc.get_event_socket_fd(), SHUT_RD);
      shutdown(m_ipc.get_main_socket_fd(), SHUT_RD);
    } catch (...) {
    }

    event_module::stop();

    // }}}
  }

  bool i3_module::has_event() {
    if (!m_ipc.handle_event())
      throw module_error("Socket connection closed...");
    return true;
  }

  bool i3_module::update() {
    // Refresh workspace data {{{

    m_workspaces.clear();
    i3_util::connection_t ipc;

    try {
      auto workspaces = ipc.get_workspaces();
      vector<shared_ptr<i3ipc::workspace_t>> sorted = workspaces;
      string focused_output;

      for (auto&& workspace : workspaces)
        if (workspace->focused) {
          focused_output = workspace->output;
          break;
        }

      if (m_indexsort) {
        using ws_t = shared_ptr<i3ipc::workspace_t>;
        // clang-format off
        sort(sorted.begin(), sorted.end(), [](ws_t ws1, ws_t ws2){
            return ws1->num < ws2->num;
        });
        // clang-format on
      }

      for (auto&& workspace : sorted) {
        if (m_pinworkspaces && workspace->output != m_bar.monitor->name)
          continue;

        auto flag = i3_flag::WORKSPACE_NONE;
        if (workspace->focused)
          flag = i3_flag::WORKSPACE_FOCUSED;
        else if (workspace->urgent)
          flag = i3_flag::WORKSPACE_URGENT;
        else if (!workspace->visible || (workspace->visible && workspace->output != focused_output))
          flag = i3_flag::WORKSPACE_UNFOCUSED;
        else
          flag = i3_flag::WORKSPACE_VISIBLE;

        string wsname{workspace->name};

        if (m_strip_wsnumbers) {
          auto index = string_util::split(wsname, ':');

          if (index.size() == 2) {
            wsname = index[1];
          }
        }

        if (m_wsname_maxlen > 0 && wsname.length() > m_wsname_maxlen)
          wsname.erase(m_wsname_maxlen);

        auto icon = m_icons->get(workspace->name, DEFAULT_WS_ICON);
        auto label = m_statelabels.find(flag)->second->clone();

        label->reset_tokens();
        label->replace_token("%output%", workspace->output);
        label->replace_token("%name%", wsname);
        label->replace_token("%icon%", icon->get());
        label->replace_token("%index%", to_string(workspace->num));
        m_workspaces.emplace_back(
            make_unique<i3_workspace>(workspace->num, flag, std::move(label)));
      }

      return true;
    } catch (const std::exception& err) {
      m_log.err("%s: %s", name(), err.what());
      return false;
    }

    // }}}
  }

  bool i3_module::build(builder* builder, string tag) const {
    // Output workspace info {{{

    if (tag != TAG_LABEL_STATE)
      return false;

    for (auto&& ws : m_workspaces) {
      builder->cmd(mousebtn::SCROLL_DOWN, EVENT_SCROLL_DOWN);
      builder->cmd(mousebtn::SCROLL_UP, EVENT_SCROLL_UP);
      builder->cmd(mousebtn::LEFT, string{EVENT_CLICK} + to_string(ws.get()->index));
      builder->node(ws.get()->label);
      builder->cmd_close(true);
    }
    return true;

    // }}}
  }

  bool i3_module::handle_event(string cmd) {
    // Send ipc commands {{{

    if (cmd.compare(0, 2, EVENT_PREFIX) != 0)
      return false;

    try {
      i3_util::connection_t ipc;

      if (cmd.compare(0, strlen(EVENT_CLICK), EVENT_CLICK) == 0) {
        m_log.info("%s: Sending workspace focus command to ipc handler", name());
        ipc.send_command("workspace number " + cmd.substr(strlen(EVENT_CLICK)));
      } else if (cmd.compare(0, strlen(EVENT_SCROLL_DOWN), EVENT_SCROLL_DOWN) == 0) {
        m_log.info("%s: Sending workspace prev command to ipc handler", name());
        ipc.send_command("workspace next_on_output");
      } else if (cmd.compare(0, strlen(EVENT_SCROLL_UP), EVENT_SCROLL_UP) == 0) {
        m_log.info("%s: Sending workspace next command to ipc handler", name());
        ipc.send_command("workspace prev_on_output");
      }
    } catch (const std::exception& err) {
      m_log.err("%s: %s", name(), err.what());
    }

    return true;

    // }}}
  }

  bool i3_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END
