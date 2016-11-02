#include <sys/socket.h>

#include "modules/bspwm.hpp"

LEMONBUDDY_NS

namespace modules {
  void bspwm_module::setup() {
    m_monitor = m_bar.monitor->name;
    m_log.trace("%s: Primary monitor '%s'", name(), m_monitor);

    // Add formats and create components {{{

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE}, {TAG_LABEL_MODE});

    if (m_formatter->has(TAG_LABEL_STATE)) {
      m_statelabels.insert(make_pair(bspwm_flag::WORKSPACE_ACTIVE,
          load_optional_label(m_conf, name(), "label-active", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(bspwm_flag::WORKSPACE_OCCUPIED,
          load_optional_label(m_conf, name(), "label-occupied", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(bspwm_flag::WORKSPACE_URGENT,
          load_optional_label(m_conf, name(), "label-urgent", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(bspwm_flag::WORKSPACE_EMPTY,
          load_optional_label(m_conf, name(), "label-empty", DEFAULT_WS_LABEL)));
      m_statelabels.insert(make_pair(
          bspwm_flag::WORKSPACE_DIMMED, load_optional_label(m_conf, name(), "label-dimmed")));
    }

    if (m_formatter->has(TAG_LABEL_MODE)) {
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_LAYOUT_MONOCLE, load_optional_label(m_conf, name(), "label-monocle")));
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_LAYOUT_TILED, load_optional_label(m_conf, name(), "label-tiled")));
      m_modelabels.insert(make_pair(bspwm_flag::MODE_STATE_FULLSCREEN,
          load_optional_label(m_conf, name(), "label-fullscreen")));
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_STATE_FLOATING, load_optional_label(m_conf, name(), "label-floating")));
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_NODE_LOCKED, load_optional_label(m_conf, name(), "label-locked")));
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_NODE_STICKY, load_optional_label(m_conf, name(), "label-sticky")));
      m_modelabels.insert(make_pair(
          bspwm_flag::MODE_NODE_PRIVATE, load_optional_label(m_conf, name(), "label-private")));
    }

    m_icons = iconset_t{new iconset()};
    m_icons->add(
        DEFAULT_WS_ICON, icon_t{new icon(m_conf.get<string>(name(), DEFAULT_WS_ICON, ""))});

    for (auto workspace : m_conf.get_list<string>(name(), "ws-icon", {})) {
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], icon_t{new icon{vec[1]}});
      }
    }

    // }}}
    // Create ipc subscriber {{{

    m_subscriber = bspwm_util::make_subscriber();

    // }}}
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

    if (bytes == 0)
      return false;

    data = string_util::strip_trailing_newline(data);

    unsigned long pos;
    while ((pos = data.find("\n")) != string::npos) data.erase(pos);

    if (data.empty())
      return false;

    const auto prefix = string{BSPWM_STATUS_PREFIX};

    // If there were more than 1 row available in the channel
    // we'll strip out the old updates
    if ((pos = data.find_last_of(prefix)) > 0)
      data = data.substr(pos);

    if (data.compare(0, prefix.length(), prefix) != 0) {
      m_log.err("%s: Unknown status '%s'", name(), data);
      return false;
    }

    unsigned long hash;
    if ((hash = string_util::hash(data)) == m_hash)
      return false;
    m_hash = hash;

    // Extract the string for the defined monitor
    const auto needle_active = ":M" + m_monitor + ":";
    const auto needle_inactive = ":m" + m_monitor + ":";

    if ((pos = data.find(prefix)) != std::string::npos)
      data = data.replace(pos, prefix.length(), ":");
    if ((pos = data.find(needle_active)) != std::string::npos)
      data.erase(0, pos + 1);
    if ((pos = data.find(needle_inactive)) != std::string::npos)
      data.erase(0, pos + 1);
    if ((pos = data.find(":m", 1)) != std::string::npos)
      data.erase(pos);
    if ((pos = data.find(":M", 1)) != std::string::npos)
      data.erase(pos);

    m_modes.clear();
    m_workspaces.clear();

    bool monitor_focused = true;
    int workspace_n = 0;

    for (auto&& tag : string_util::split(data, ':')) {
      if (tag.empty())
        continue;

      auto value = tag.size() > 0 ? tag.substr(1) : "";
      auto workspace_flag = bspwm_flag::WORKSPACE_NONE;
      auto mode_flag = bspwm_flag::MODE_NONE;

      switch (tag[0]) {
        case 'm':
          monitor_focused = false;
          break;
        case 'M':
          monitor_focused = true;
          break;
        case 'F':
          workspace_flag = bspwm_flag::WORKSPACE_ACTIVE;
          break;
        case 'O':
          workspace_flag = bspwm_flag::WORKSPACE_ACTIVE;
          break;
        case 'o':
          workspace_flag = bspwm_flag::WORKSPACE_OCCUPIED;
          break;
        case 'U':
          workspace_flag = bspwm_flag::WORKSPACE_URGENT;
          break;
        case 'u':
          workspace_flag = bspwm_flag::WORKSPACE_URGENT;
          break;
        case 'f':
          workspace_flag = bspwm_flag::WORKSPACE_EMPTY;
          break;
        case 'L':
          switch (value[0]) {
            case 0:
              break;
            case 'M':
              mode_flag = bspwm_flag::MODE_LAYOUT_MONOCLE;
              break;
            case 'T':
              mode_flag = bspwm_flag::MODE_LAYOUT_TILED;
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
              mode_flag = bspwm_flag::MODE_STATE_FULLSCREEN;
              break;
            case 'F':
              mode_flag = bspwm_flag::MODE_STATE_FLOATING;
              break;
            default:
              m_log.warn("%s: Undefined T => '%s'", name(), value);
          }
          break;

        case 'G':
          for (int i = 0; i < (int)value.length(); i++) {
            switch (value[i]) {
              case 0:
                break;
              case 'L':
                mode_flag = bspwm_flag::MODE_NODE_LOCKED;
                break;
              case 'S':
                mode_flag = bspwm_flag::MODE_NODE_STICKY;
                break;
              case 'P':
                mode_flag = bspwm_flag::MODE_NODE_PRIVATE;
                break;
              default:
                m_log.warn("%s: Undefined G => '%s'", name(), value.substr(i, 1));
            }

            if (mode_flag != bspwm_flag::MODE_NONE && !m_modelabels.empty())
              m_modes.emplace_back(m_modelabels.find(mode_flag)->second->clone());
          }
          continue;

        default:
          m_log.warn("%s: Undefined tag => '%s'", name(), tag.substr(0, 1));
      }

      if (workspace_flag != bspwm_flag::WORKSPACE_NONE && m_formatter->has(TAG_LABEL_STATE)) {
        auto icon = m_icons->get(value, DEFAULT_WS_ICON);
        auto label = m_statelabels.find(workspace_flag)->second->clone();

        if (!monitor_focused)
          label->replace_defined_values(m_statelabels.find(bspwm_flag::WORKSPACE_DIMMED)->second);

        label->reset_tokens();
        label->replace_token("%name%", value);
        label->replace_token("%icon%", icon->get());
        label->replace_token("%index%", to_string(++workspace_n));

        m_workspaces.emplace_back(make_unique<bspwm_workspace>(workspace_flag, std::move(label)));
      }

      if (mode_flag != bspwm_flag::MODE_NONE && !m_modelabels.empty())
        m_modes.emplace_back(m_modelabels.find(mode_flag)->second->clone());
    }

    if (!monitor_focused)
      m_modes.clear();

    return true;
  }

  bool bspwm_module::build(builder* builder, string tag) const {
    if (tag != TAG_LABEL_STATE)
      return false;

    int workspace_n = 0;

    for (auto&& ws : m_workspaces) {
      if (!ws.get()->label->get().empty())
        builder->cmd(mousebtn::LEFT, string(EVENT_CLICK) + to_string(++workspace_n));

      builder->node(ws.get()->label);

      if (ws->flag == bspwm_flag::WORKSPACE_ACTIVE && m_formatter->has(TAG_LABEL_MODE)) {
        for (auto&& mode : m_modes) builder->node(mode);
      }

      if (!ws.get()->label->get().empty())
        builder->cmd_close(true);
    }

    return true;
  }

  bool bspwm_module::handle_event(string cmd) {
    if (cmd.find(EVENT_CLICK) == string::npos || cmd.length() <= strlen(EVENT_CLICK))
      return false;

    try {
      auto ipc = bspwm_util::make_connection();
      auto payload = bspwm_util::make_payload(
          "desktop -f " + m_monitor + ":^" + cmd.substr(strlen(EVENT_CLICK)));

      m_log.info("%s: Sending desktop focus command to ipc handler", name());

      ipc->send(payload->data, payload->len, 0);
      ipc->disconnect();
    } catch (const system_error& err) {
      m_log.err("%s: %s", name(), err.what());
    }

    return true;
  }

  bool bspwm_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END
