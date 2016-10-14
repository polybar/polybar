#pragma once

#include <i3ipc++/ipc.hpp>

#include "config.hpp"
#include "components/config.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"
#include "utils/i3.hpp"
#include "utils/io.hpp"

LEMONBUDDY_NS

namespace modules {
  // meta types {{{

  enum class i3_flag {
    WORKSPACE_NONE,
    WORKSPACE_FOCUSED,
    WORKSPACE_UNFOCUSED,
    WORKSPACE_VISIBLE,
    WORKSPACE_URGENT,
  };

  struct i3_workspace {
    int index;
    i3_flag flag;
    label_t label;

    i3_workspace(int index_, i3_flag flag_, label_t&& label_)
        : index(index_), flag(flag_), label(forward<decltype(label_)>(label_)) {}

    operator bool() {
      return label && *label;
    }
  };

  using i3_workspace_t = unique_ptr<i3_workspace>;

  // }}}

  class i3_module : public event_module<i3_module> {
   public:
    using event_module::event_module;

    ~i3_module() {
      // Shutdown ipc connection {{{

      try {
        shutdown(m_ipc.get_event_socket_fd(), SHUT_RD);
        shutdown(m_ipc.get_main_socket_fd(), SHUT_RD);
      } catch (const std::exception& err) {
      }

      // }}}
    }

    void setup() {
      // Load configuration values {{{

      GET_CONFIG_VALUE(name(), m_indexsort, "index-sort");
      GET_CONFIG_VALUE(name(), m_pinworkspaces, "pin-workspaces");
      GET_CONFIG_VALUE(name(), m_wsname_maxlen, "wsname-maxlen");

      // }}}
      // Add formats and create components {{{

      m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE});

      if (m_formatter->has(TAG_LABEL_STATE)) {
        m_statelabels.insert(make_pair(i3_flag::WORKSPACE_FOCUSED,
            get_optional_config_label(m_conf, name(), "label-focused", DEFAULT_WS_LABEL)));
        m_statelabels.insert(make_pair(i3_flag::WORKSPACE_UNFOCUSED,
            get_optional_config_label(m_conf, name(), "label-unfocused", DEFAULT_WS_LABEL)));
        m_statelabels.insert(make_pair(i3_flag::WORKSPACE_VISIBLE,
            get_optional_config_label(m_conf, name(), "label-visible", DEFAULT_WS_LABEL)));
        m_statelabels.insert(make_pair(i3_flag::WORKSPACE_URGENT,
            get_optional_config_label(m_conf, name(), "label-urgent", DEFAULT_WS_LABEL)));
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
      } catch (std::runtime_error& e) {
        throw module_error(e.what());
      }

      // }}}
    }

    bool has_event() {
      // Wait for ipc events {{{

      try {
        m_ipc.handle_event();
        return true;
      } catch (const std::exception& err) {
        m_log.err("%s: Error while handling ipc event, stopping module...", name());
        m_log.err("%s: %s", name(), err.what());
        stop();
        return false;
      }

      // }}}
    }

    bool update() {
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
          if (m_wsname_maxlen > 0 && wsname.length() > m_wsname_maxlen)
            wsname.erase(m_wsname_maxlen);

          auto icon = m_icons->get(workspace->name, DEFAULT_WS_ICON);
          auto label = m_statelabels.find(flag)->second->clone();

          label->replace_token("%output%", workspace->output);
          label->replace_token("%name%", wsname);
          label->replace_token("%icon%", icon->m_text);
          label->replace_token("%index%", to_string(workspace->num));
          m_workspaces.emplace_back(make_unique<i3_workspace>(workspace->num, flag, std::move(label)));
        }

        return true;
      } catch (const std::exception& err) {
        m_log.err("%s: %s", name(), err.what());
        return false;
      }

      // }}}
    }

    bool build(builder* builder, string tag) {
      // Output workspace info {{{

      if (tag != TAG_LABEL_STATE)
        return false;
      for (auto&& ws : m_workspaces) {
        builder->cmd(mousebtn::LEFT, string{EVENT_CLICK} + to_string(ws.get()->index));
        builder->node(ws.get()->label);
        builder->cmd_close(true);
      }
      return true;

      // }}}
    }

    bool handle_event(string cmd) {
      // Send ipc commands {{{

      if (cmd.find(EVENT_CLICK) == string::npos)
        return false;
      if (cmd.length() < strlen(EVENT_CLICK))
        return false;

      try {
        i3_util::connection_t ipc;
        m_log.info("%s: Sending workspace focus command to ipc handler", name());
        ipc.send_command("workspace number "+ cmd.substr(strlen(EVENT_CLICK)));
      } catch (const std::exception& err) {
        m_log.err("%s: %s", name(), err.what());
      }

      return true;

      // }}}
    }

    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto DEFAULT_WS_ICON = "ws-icon-default";
    static constexpr auto DEFAULT_WS_LABEL = "%icon% %name%";
    static constexpr auto TAG_LABEL_STATE = "<label-state>";
    static constexpr auto EVENT_CLICK = "i3-wsfocus-";

    map<i3_flag, label_t> m_statelabels;
    vector<i3_workspace_t> m_workspaces;
    iconset_t m_icons;

    bool m_indexsort = false;
    bool m_pinworkspaces = false;
    size_t m_wsname_maxlen = 0;

    i3_util::connection_t m_ipc;
  };
}

LEMONBUDDY_NS_END
