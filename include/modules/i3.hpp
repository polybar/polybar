#pragma once

// #include <i3ipc++/ipc-util.hpp>
// #include <i3ipc++/ipc.hpp>

#include "config.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class i3_flag {
    WORKSPACE_NONE,
    WORKSPACE_FOCUSED,
    WORKSPACE_UNFOCUSED,
    WORKSPACE_VISIBLE,
    WORKSPACE_URGENT,
    WORKSPACE_DIMMED,  // used when the monitor is out of focus
  };

  struct i3_workspace {
    explicit i3_workspace(int idx, i3_flag flag, label_t&& label_)
        : idx(idx), flag(flag), label(forward<decltype(label_)>(label_)) {}

    operator bool() {
      return label && label.get();
    }

    int idx;
    i3_flag flag;
    label_t label;
  };

  namespace i3ipc {
    static const int ET_WORKSPACE = 1;
    static const int ET_OUTPUT = 2;
    static const int ET_WINDOW = 4;

    class ws {
     public:
      bool focused;
      bool urgent;
      bool visible;
      string output;
      string name;
      unsigned int num;
    };

    class connection {
     public:
      void subscribe(int) {}
      void send_command(string) {}
      void prepare_to_event_handling() {}
      void handle_event() {}
      vector<unique_ptr<ws>> get_workspaces() {
        return {};
      }
    };
  }

  using i3_workspace_t = unique_ptr<i3_workspace>;

  class i3_module : public event_module<i3_module> {
   public:
    using event_module::event_module;

    ~i3_module() {
      if (!m_ipc)
        return;
      // FIXME: Hack to release the recv lock. Will need to patch the ipc lib
      m_ipc->send_command("workspace back_and_forth");
      m_ipc->send_command("workspace back_and_forth");
    }

    void setup() {
      try {
        m_ipc = make_unique<i3ipc::connection>();
      } catch (std::runtime_error& e) {
        throw module_error(e.what());
      }

      m_localworkspaces = m_conf.get<bool>(name(), "local_workspaces", m_localworkspaces);
      m_workspace_name_strip_nchars =
          m_conf.get<size_t>(name(), "workspace_name_strip_nchars", m_workspace_name_strip_nchars);

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
        m_statelabels.insert(make_pair(
            i3_flag::WORKSPACE_DIMMED, get_optional_config_label(m_conf, name(), "label-dimmed")));
      }

      m_icons = iconset_t{new iconset()};
      m_icons->add(
          DEFAULT_WS_ICON, icon_t{new icon(m_conf.get<string>(name(), DEFAULT_WS_ICON, ""))});

      for (auto workspace : m_conf.get_list<string>(name(), "workspace_icon", {})) {
        auto vec = string_util::split(workspace, ';');
        if (vec.size() == 2)
          m_icons->add(vec[0], icon_t{new icon{vec[1]}});
      }

      // m_ipc->subscribe(i3ipc::ET_WORKSPACE | i3ipc::ET_OUTPUT | i3ipc::ET_WINDOW);
      // m_ipc->subscribe(i3ipc::ET_WORKSPACE | i3ipc::ET_OUTPUT);
      // m_ipc->prepare_to_event_handling();
    }

    bool has_event() {
      if (!m_ipc || !enabled())
        return false;
      m_ipc->handle_event();
      return true;
    }

    bool update() {
      if (!enabled())
        return false;

      i3ipc::connection connection;

      try {
        // for (auto &&m : connection.get_outputs()) {
        //   if (m->name == m_monitor) {
        //     monitor_focused = m->active;
        //     break;
        //   }
        // }

        m_workspaces.clear();

        auto workspaces = connection.get_workspaces();

        string focused_monitor;

        for (auto&& ws : workspaces) {
          if (ws->focused) {
            focused_monitor = ws->output;
            break;
          }
        }

        bool monitor_focused = (focused_monitor == m_monitor);

        for (auto&& ws : connection.get_workspaces()) {
          if (m_localworkspaces && ws->output != m_monitor)
            continue;

          auto flag = i3_flag::WORKSPACE_NONE;

          if (ws->focused)
            flag = i3_flag::WORKSPACE_FOCUSED;
          else if (ws->urgent)
            flag = i3_flag::WORKSPACE_URGENT;
          else if (ws->visible)
            flag = i3_flag::WORKSPACE_VISIBLE;
          else
            flag = i3_flag::WORKSPACE_UNFOCUSED;

          // if (!monitor_focused)
          //   flag = i3_flag::WORKSPACE_DIMMED;

          auto workspace_name = ws->name;
          if (m_workspace_name_strip_nchars > 0 &&
              workspace_name.length() > m_workspace_name_strip_nchars)
            workspace_name.erase(0, m_workspace_name_strip_nchars);

          auto icon = m_icons->get(workspace_name, DEFAULT_WS_ICON);
          auto label = m_statelabels.find(flag)->second->clone();

          if (!monitor_focused)
            label->replace_defined_values(m_statelabels.find(i3_flag::WORKSPACE_DIMMED)->second);

          label->replace_token("%name%", workspace_name);
          label->replace_token("%icon%", icon->m_text);
          label->replace_token("%index%", to_string(ws->num));

          m_workspaces.emplace_back(make_unique<i3_workspace>(ws->num, flag, std::move(label)));
        }
      } catch (std::runtime_error& e) {
        m_log.err("%s: %s", name(), e.what());
      }

      return true;
    }

    bool build(builder* builder, string tag) {
      if (tag != TAG_LABEL_STATE)
        return false;

      for (auto&& ws : m_workspaces) {
        builder->cmd(mousebtn::LEFT, string{EVENT_CLICK} + to_string(ws.get()->idx));
        builder->node(ws.get()->label);
        builder->cmd_close(true);
      }

      return true;
    }

    bool handle_event(string cmd) {
      if (cmd.find(EVENT_CLICK) == string::npos || cmd.length() < strlen(EVENT_CLICK))
        return false;

      m_ipc->send_command("workspace number " + cmd.substr(strlen(EVENT_CLICK)));

      return true;
    }

    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto DEFAULT_WS_ICON = "workspace_icon-default";
    static constexpr auto DEFAULT_WS_LABEL = "%icon% %name%";

    static constexpr auto TAG_LABEL_STATE = "<label-state>";

    static constexpr auto EVENT_CLICK = "i3";

    unique_ptr<i3ipc::connection> m_ipc;

    map<i3_flag, label_t> m_statelabels;
    vector<i3_workspace_t> m_workspaces;

    // map<i3_flag, label_t> mode_labels;
    // vector<label_t> modes;

    iconset_t m_icons;
    string m_monitor;

    bool m_localworkspaces = true;
    size_t m_workspace_name_strip_nchars = 0;
  };
}

LEMONBUDDY_NS_END
