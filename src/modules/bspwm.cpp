#include <sys/socket.h>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/bspwm.hpp"
#include "utils/file.hpp"
#include "utils/factory.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/event_module.inl"

POLYBAR_NS

namespace {
  using bspwm_state = modules::bspwm_module::state;

  uint32_t make_mask(bspwm_state s1, bspwm_state s2 = bspwm_state::NONE) {
    uint32_t mask{0U};
    if (static_cast<uint32_t>(s1)) {
      mask |= 1 << (static_cast<uint32_t>(s1) - 1);
    }
    if (static_cast<uint32_t>(s2)) {
      mask |= 1 << (static_cast<uint32_t>(s2) - 1);
    }
    return mask;
  }

  // uint32_t check_mask(uint32_t base, bspwm_state s1, bspwm_state s2 = bspwm_state::NONE) {
  //   uint32_t mask{0U};
  //   if (static_cast<uint32_t>(s1))
  //     mask |= 1 << (static_cast<uint32_t>(s1) - 1);
  //   if (static_cast<uint32_t>(s2))
  //     mask |= 1 << (static_cast<uint32_t>(s2) - 1);
  //   return (base & mask) == mask;
  // }
}

namespace modules {
  template class module<bspwm_module>;
  template class event_module<bspwm_module>;

  void bspwm_module::setup() {
    auto socket_path = bspwm_util::get_socket_path();

    if (!file_util::exists(socket_path)) {
      throw module_error("Could not find socket: " + (socket_path.empty() ? "<empty>" : socket_path));
    }

    // Create ipc subscriber
    m_subscriber = bspwm_util::make_subscriber();

    // Load configuration values
    GET_CONFIG_VALUE(name(), m_pinworkspaces, "pin-workspaces");
    GET_CONFIG_VALUE(name(), m_click, "enable-click");
    GET_CONFIG_VALUE(name(), m_scroll, "enable-scroll");

    // Add formats and create components
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE}, {TAG_LABEL_MONITOR, TAG_LABEL_MODE});

    if (m_formatter->has(TAG_LABEL_MONITOR)) {
      m_monitorlabel = load_optional_label(m_conf, name(), "label-monitor", DEFAULT_MONITOR_LABEL);
    }

    if (m_formatter->has(TAG_LABEL_STATE)) {
      // XXX: Warn about deprecated parameters
      m_conf.warn_deprecated(name(), "label-dimmed-active", "label-dimmed-focused");

      // clang-format off
      try {
        m_statelabels.emplace(make_mask(state::FOCUSED), load_label(m_conf, name(), "label-active", DEFAULT_LABEL));
        m_conf.warn_deprecated(name(), "label-active", "label-focused and label-dimmed-focused");
      } catch (const key_error& err) {
        m_statelabels.emplace(make_mask(state::FOCUSED), load_optional_label(m_conf, name(), "label-focused", DEFAULT_LABEL));
      }

      m_statelabels.emplace(make_mask(state::OCCUPIED),
          load_optional_label(m_conf, name(), "label-occupied", DEFAULT_LABEL));
      m_statelabels.emplace(make_mask(state::URGENT),
          load_optional_label(m_conf, name(), "label-urgent", DEFAULT_LABEL));
      m_statelabels.emplace(make_mask(state::EMPTY),
          load_optional_label(m_conf, name(), "label-empty", DEFAULT_LABEL));
      m_statelabels.emplace(make_mask(state::DIMMED),
          load_optional_label(m_conf, name(), "label-dimmed"));

      vector<pair<state, string>> focused_overrides{
        {state::OCCUPIED, "label-focused-occupied"},
        {state::URGENT, "label-focused-urgent"},
        {state::EMPTY, "label-focused-empty"}};

      for (auto&& os : focused_overrides) {
        uint32_t mask{make_mask(state::FOCUSED, os.first)};
        try {
          m_statelabels.emplace(mask, load_label(m_conf, name(), os.second));
        } catch (const key_error& err) {
          m_statelabels.emplace(mask, m_statelabels.at(make_mask(state::FOCUSED))->clone());
        }
      }

      vector<pair<state, string>> dimmed_overrides{
        {state::FOCUSED, "label-dimmed-focused"},
        {state::OCCUPIED, "label-dimmed-occupied"},
        {state::URGENT, "label-dimmed-urgent"},
        {state::EMPTY, "label-dimmed-empty"}};

      for (auto&& os : dimmed_overrides) {
        m_statelabels.emplace(make_mask(state::DIMMED, os.first),
            load_optional_label(m_conf, name(), os.second, m_statelabels.at(make_mask(os.first))->get()));
      }
      // clang-format on
    }

    if (m_formatter->has(TAG_LABEL_MODE)) {
      m_modelabels.emplace(mode::LAYOUT_MONOCLE, load_optional_label(m_conf, name(), "label-monocle"));
      m_modelabels.emplace(mode::LAYOUT_TILED, load_optional_label(m_conf, name(), "label-tiled"));
      m_modelabels.emplace(mode::STATE_FULLSCREEN, load_optional_label(m_conf, name(), "label-fullscreen"));
      m_modelabels.emplace(mode::STATE_FLOATING, load_optional_label(m_conf, name(), "label-floating"));
      m_modelabels.emplace(mode::NODE_LOCKED, load_optional_label(m_conf, name(), "label-locked"));
      m_modelabels.emplace(mode::NODE_STICKY, load_optional_label(m_conf, name(), "label-sticky"));
      m_modelabels.emplace(mode::NODE_PRIVATE, load_optional_label(m_conf, name(), "label-private"));
    }

    m_icons = factory_util::shared<iconset>();
    m_icons->add(DEFAULT_ICON, factory_util::shared<label>(m_conf.get<string>(name(), DEFAULT_ICON, "")));

    for (const auto& workspace : m_conf.get_list<string>(name(), "ws-icon", {})) {
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], factory_util::shared<label>(vec[1]));
      }
    }
  }

  void bspwm_module::stop() {
    if (m_subscriber) {
      m_log.info("%s: Disconnecting from socket", name());
      m_subscriber->disconnect();
    }
    event_module::stop();
  }

  bool bspwm_module::has_event() {
    if (m_subscriber->poll(POLLHUP, 0)) {
      m_log.warn("%s: Reconnecting to socket...", name());
      m_subscriber = bspwm_util::make_subscriber();
    }

    ssize_t bytes = 0;
    m_subscriber->receive(1, bytes, MSG_PEEK);
    return bytes > 0;
  }

  bool bspwm_module::update() {
    ssize_t bytes = 0;
    string data = m_subscriber->receive(BUFSIZ - 1, bytes, 0);

    if (bytes == 0) {
      return false;
    }

    data = string_util::strip_trailing_newline(data);

    unsigned long pos;
    while ((pos = data.find('\n')) != string::npos) {
      data.erase(pos);
    }

    if (data.empty()) {
      return false;
    }

    const auto prefix = string{BSPWM_STATUS_PREFIX};

    // If there were more than 1 row available in the channel
    // we'll strip out the old updates
    if ((pos = data.find_last_of(prefix)) > 0) {
      data = data.substr(pos);
    }

    if (data.compare(0, prefix.length(), prefix) != 0) {
      m_log.err("%s: Unknown status '%s'", name(), data);
      return false;
    }

    unsigned long hash;
    if ((hash = string_util::hash(data)) == m_hash) {
      return false;
    }
    m_hash = hash;

    // Extract the string for the defined monitor
    if (m_pinworkspaces) {
      const auto needle_active = ":M" + m_bar.monitor->name + ":";
      const auto needle_inactive = ":m" + m_bar.monitor->name + ":";

      if ((pos = data.find(prefix)) != string::npos) {
        data = data.replace(pos, prefix.length(), ":");
      }
      if ((pos = data.find(needle_active)) != string::npos) {
        data.erase(0, pos + 1);
      }
      if ((pos = data.find(needle_inactive)) != string::npos) {
        data.erase(0, pos + 1);
      }
      if ((pos = data.find(":m", 1)) != string::npos) {
        data.erase(pos);
      }
      if ((pos = data.find(":M", 1)) != string::npos) {
        data.erase(pos);
      }
    } else if ((pos = data.find(prefix)) != string::npos) {
      data = data.replace(pos, prefix.length(), ":");
    } else {
      return false;
    }

    int workspace_n = 0;

    m_monitors.clear();

    for (auto&& tag : string_util::split(data, ':')) {
      if (tag.empty()) {
        continue;
      }

      auto value = !tag.empty() ? tag.substr(1) : "";
      auto mode_flag = mode::NONE;
      uint32_t workspace_mask{0U};

      if (tag[0] == 'm' || tag[0] == 'M') {
        m_monitors.emplace_back(factory_util::unique<bspwm_monitor>());
        m_monitors.back()->name = value;

        if (m_monitorlabel) {
          m_monitors.back()->label = m_monitorlabel->clone();
          m_monitors.back()->label->replace_token("%name%", value);
        }
      }

      switch (tag[0]) {
        case 'm':
          m_monitors.back()->focused = false;
          break;
        case 'M':
          m_monitors.back()->focused = true;
          break;
        case 'F':
          workspace_mask = make_mask(state::FOCUSED, state::EMPTY);
          break;
        case 'O':
          workspace_mask = make_mask(state::FOCUSED, state::OCCUPIED);
          break;
        case 'U':
          workspace_mask = make_mask(state::FOCUSED, state::URGENT);
          break;
        case 'f':
          workspace_mask = make_mask(state::EMPTY);
          break;
        case 'o':
          workspace_mask = make_mask(state::OCCUPIED);
          break;
        case 'u':
          workspace_mask = make_mask(state::URGENT);
          break;
        case 'L':
          switch (value[0]) {
            case 0:
              break;
            case 'M':
              mode_flag = mode::LAYOUT_MONOCLE;
              break;
            case 'T':
              mode_flag = mode::LAYOUT_TILED;
              break;
            default:
              m_log.warn("%s: Undefined L => '%s'", name(), value);
          }
          break;

        case 'T':
          switch (value[0]) {
            case 0:
              break;
            case 'T':
              break;
            case '=':
              mode_flag = mode::STATE_FULLSCREEN;
              break;
            case 'F':
              mode_flag = mode::STATE_FLOATING;
              break;
            default:
              m_log.warn("%s: Undefined T => '%s'", name(), value);
          }
          break;

        case 'G':
          if (!m_monitors.back()->focused) {
            break;
          }

          for (int i = 0; i < (int)value.length(); i++) {
            switch (value[i]) {
              case 0:
                break;
              case 'L':
                mode_flag = mode::NODE_LOCKED;
                break;
              case 'S':
                mode_flag = mode::NODE_STICKY;
                break;
              case 'P':
                mode_flag = mode::NODE_PRIVATE;
                break;
              default:
                m_log.warn("%s: Undefined G => '%s'", name(), value.substr(i, 1));
            }

            if (mode_flag != mode::NONE && !m_modelabels.empty()) {
              m_monitors.back()->modes.emplace_back(m_modelabels.find(mode_flag)->second->clone());
            }
          }
          continue;

        default:
          m_log.warn("%s: Undefined tag => '%s'", name(), tag.substr(0, 1));
      }

      if (workspace_mask && m_formatter->has(TAG_LABEL_STATE)) {
        auto icon = m_icons->get(value, DEFAULT_ICON);
        auto label = m_statelabels.at(workspace_mask)->clone();

        if (!m_monitors.back()->focused) {
          if (m_statelabels[make_mask(state::DIMMED)]) {
            label->replace_defined_values(m_statelabels[make_mask(state::DIMMED)]);
          }
          if (workspace_mask & make_mask(state::EMPTY)) {
            label->replace_defined_values(m_statelabels[make_mask(state::DIMMED, state::EMPTY)]);
          }
          if (workspace_mask & make_mask(state::OCCUPIED)) {
            label->replace_defined_values(m_statelabels[make_mask(state::DIMMED, state::OCCUPIED)]);
          }
          if (workspace_mask & make_mask(state::FOCUSED)) {
            label->replace_defined_values(m_statelabels[make_mask(state::DIMMED, state::FOCUSED)]);
          }
          if (workspace_mask & make_mask(state::URGENT)) {
            label->replace_defined_values(m_statelabels[make_mask(state::DIMMED, state::URGENT)]);
          }
        }

        label->reset_tokens();
        label->replace_token("%name%", value);
        label->replace_token("%icon%", icon->get());
        label->replace_token("%index%", to_string(++workspace_n));

        m_monitors.back()->workspaces.emplace_back(workspace_mask, move(label));
      }

      if (mode_flag != mode::NONE && !m_modelabels.empty()) {
        m_monitors.back()->modes.emplace_back(m_modelabels.find(mode_flag)->second->clone());
      }
    }

    return true;
  }

  string bspwm_module::get_output() {
    string output;
    for (m_index = 0; m_index < m_monitors.size(); m_index++) {
      if (m_index > 0) {
        m_builder->space(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += event_module::get_output();
    }
    return output;
  }

  bool bspwm_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_MONITOR) {
      builder->node(m_monitors[m_index]->label);
      return true;
    } else if (tag == TAG_LABEL_STATE && !m_monitors[m_index]->workspaces.empty()) {
      int workspace_n = 0;

      if (m_scroll) {
        builder->cmd(mousebtn::SCROLL_DOWN, EVENT_SCROLL_DOWN);
        builder->cmd(mousebtn::SCROLL_UP, EVENT_SCROLL_UP);
      }

      for (auto&& ws : m_monitors[m_index]->workspaces) {
        if (ws.second.get()) {
          if (m_click) {
            builder->cmd(mousebtn::LEFT, EVENT_CLICK + to_string(m_index) + "+" + to_string(++workspace_n));
            builder->node(ws.second);
            builder->cmd_close();
          } else {
            workspace_n++;
            builder->node(ws.second);
          }
        }
      }

      if (m_scroll) {
        builder->cmd_close();
        builder->cmd_close();
      }

      return workspace_n > 0;
    } else if (tag == TAG_LABEL_MODE && m_monitors[m_index]->focused && !m_monitors[m_index]->modes.empty()) {
      int modes_n = 0;

      for (auto&& mode : m_monitors[m_index]->modes) {
        if (*mode.get()) {
          builder->node(mode);
          modes_n++;
        }
      }

      return modes_n > 0;
    }

    return false;
  }

  bool bspwm_module::handle_event(string cmd) {
    if (cmd.find(EVENT_PREFIX) != 0) {
      return false;
    }

    auto send_command = [this](string payload_cmd, string log_info) {
      try {
        auto ipc = bspwm_util::make_connection();
        auto payload = bspwm_util::make_payload(payload_cmd);
        m_log.info("%s: %s", name(), log_info);
        ipc->send(payload->data, payload->len, 0);
        ipc->disconnect();
      } catch (const system_error& err) {
        m_log.err("%s: %s", name(), err.what());
      }
    };

    string modifier;

    if (m_pinworkspaces) {
      modifier = ".local";
    }

    if (cmd.compare(0, strlen(EVENT_CLICK), EVENT_CLICK) == 0) {
      cmd.erase(0, strlen(EVENT_CLICK));

      size_t separator = string_util::find_nth(cmd, 0, "+", 1);
      size_t monitor_n = std::atoi(cmd.substr(0, separator).c_str());
      string workspace_n = cmd.substr(separator + 1);

      if (monitor_n < m_monitors.size()) {
        send_command("desktop -f " + m_monitors[monitor_n]->name + ":^" + workspace_n,
            "Sending desktop focus command to ipc handler");
      } else {
        m_log.err("%s: Invalid monitor index in command: %s", name(), cmd);
      }
    } else if (cmd.compare(0, strlen(EVENT_SCROLL_UP), EVENT_SCROLL_UP) == 0) {
      send_command("desktop -f next" + modifier, "Sending desktop next command to ipc handler");
    } else if (cmd.compare(0, strlen(EVENT_SCROLL_DOWN), EVENT_SCROLL_DOWN) == 0) {
      send_command("desktop -f prev" + modifier, "Sending desktop prev command to ipc handler");
    }

    return true;
  }
}

POLYBAR_NS_END
