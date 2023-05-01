#include "modules/bspwm.hpp"

#include <sys/socket.h>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace {
  using bspwm_state = modules::bspwm_module::state;

  unsigned int make_mask(bspwm_state s1, bspwm_state s2 = bspwm_state::NONE) {
    unsigned int mask{0U};
    if (static_cast<unsigned int>(s1)) {
      mask |= 1U << (static_cast<unsigned int>(s1) - 1U);
    }
    if (static_cast<unsigned int>(s2)) {
      mask |= 1U << (static_cast<unsigned int>(s2) - 1U);
    }
    return mask;
  }

  unsigned int check_mask(unsigned int base, bspwm_state s1, bspwm_state s2 = bspwm_state::NONE) {
    unsigned int mask{0U};
    if (static_cast<unsigned int>(s1)) {
      mask |= 1U << (static_cast<unsigned int>(s1) - 1U);
    }
    if (static_cast<unsigned int>(s2)) {
      mask |= 1U << (static_cast<unsigned int>(s2) - 1U);
    }
    return (base & mask) == mask;
  }
}  // namespace

namespace modules {
  template class module<bspwm_module>;

  bspwm_module::bspwm_module(const bar_settings& bar, string name_, const config& config)
      : event_module<bspwm_module>(bar, move(name_), config) {
    m_router->register_action_with_data(EVENT_FOCUS, [this](const std::string& data) { action_focus(data); });
    m_router->register_action(EVENT_NEXT, [this]() { action_next(); });
    m_router->register_action(EVENT_PREV, [this]() { action_prev(); });

    auto socket_path = bspwm_util::get_socket_path();

    if (!file_util::exists(socket_path)) {
      throw module_error("Could not find socket: " + (socket_path.empty() ? "<empty>" : socket_path));
    }

    // Create ipc subscriber
    m_subscriber = bspwm_util::make_subscriber();

    // Load configuration values
    m_pinworkspaces = m_conf.get(name(), "pin-workspaces", m_pinworkspaces);
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);
    m_occscroll = m_conf.get(name(), "occupied-scroll", m_occscroll);
    m_revscroll = m_conf.get(name(), "reverse-scroll", m_revscroll);
    m_inlinemode = m_conf.get(name(), "inline-mode", m_inlinemode);
    m_fuzzy_match = m_conf.get(name(), "fuzzy-match", m_fuzzy_match);

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
        unsigned int mask{make_mask(state::FOCUSED, os.first)};
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
      m_modelabels.emplace(mode::STATE_PSEUDOTILED, load_optional_label(m_conf, name(), "label-pseudotiled"));
      m_modelabels.emplace(mode::NODE_LOCKED, load_optional_label(m_conf, name(), "label-locked"));
      m_modelabels.emplace(mode::NODE_STICKY, load_optional_label(m_conf, name(), "label-sticky"));
      m_modelabels.emplace(mode::NODE_PRIVATE, load_optional_label(m_conf, name(), "label-private"));
      m_modelabels.emplace(mode::NODE_MARKED, load_optional_label(m_conf, name(), "label-marked"));
    }

    m_labelseparator = load_optional_label(m_conf, name(), "label-separator", "");

    m_icons = std::make_shared<iconset>();
    m_icons->add(DEFAULT_ICON, std::make_shared<label>(m_conf.get(name(), DEFAULT_ICON, ""s)));

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
      m_log.notice("%s: Reconnecting to socket...", name());
      m_subscriber = bspwm_util::make_subscriber();
    }
    return m_subscriber->peek(1);
  }

  bool bspwm_module::update() {
    if (!m_subscriber) {
      return false;
    }

    string data{m_subscriber->receive(BUFSIZ)};
    bool result = false;

    for (auto&& status_line : string_util::split(data, '\n')) {
      // Need to return true if ANY of the handle_status calls
      // return true
      result = this->handle_status(status_line) || result;
    }

    return result;
  }

  bool bspwm_module::handle_status(string& data) {
    if (data.empty()) {
      return false;
    }

    size_t prefix_len{strlen(BSPWM_STATUS_PREFIX)};
    if (data.compare(0, prefix_len, BSPWM_STATUS_PREFIX) != 0) {
      m_log.err("%s: Unknown status '%s'", name(), data);
      return false;
    }

    string_util::hash_type hash;
    if ((hash = string_util::hash(data)) == m_hash) {
      return false;
    }

    m_hash = hash;

    size_t pos;

    // Extract the string for the defined monitor
    if (m_pinworkspaces) {
      const auto needle_active = ":M" + m_bar.monitor->name + ":";
      const auto needle_inactive = ":m" + m_bar.monitor->name + ":";

      if ((pos = data.find(BSPWM_STATUS_PREFIX)) != string::npos) {
        data = data.replace(pos, prefix_len, ":");
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
    } else if ((pos = data.find(BSPWM_STATUS_PREFIX)) != string::npos) {
      data = data.replace(pos, prefix_len, ":");
    } else {
      return false;
    }

    m_log.info("%s: Parsing socket data: %s", name(), data);

    m_monitors.clear();

    size_t workspace_n{0U};

    for (auto&& tag : string_util::split(data, ':')) {
      auto value = tag.substr(1);
      auto mode_flag = mode::NONE;
      unsigned int workspace_mask{0U};

      if (tag[0] == 'm' || tag[0] == 'M') {
        m_monitors.emplace_back(std::make_unique<bspwm_monitor>());
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
            case '@':
              break;
            case 'T':
              break;
            case '=':
              mode_flag = mode::STATE_FULLSCREEN;
              break;
            case 'F':
              mode_flag = mode::STATE_FLOATING;
              break;
            case 'P':
              mode_flag = mode::STATE_PSEUDOTILED;
              break;
            default:
              m_log.warn("%s: Undefined T => '%s'", name(), value);
          }
          break;

        case 'G':
          if (!m_monitors.back()->focused) {
            break;
          }

          for (size_t i = 0U; i < value.length(); i++) {
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
              case 'M':
                mode_flag = mode::NODE_MARKED;
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
          continue;
      }

      if (!m_monitors.back()) {
        m_log.warn("%s: No monitor created", name());
        continue;
      }

      if (workspace_mask && m_formatter->has(TAG_LABEL_STATE)) {
        auto icon = m_icons->get(value, DEFAULT_ICON, m_fuzzy_match);
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
    for (m_index = 0U; m_index < m_monitors.size(); m_index++) {
      if (m_index > 0) {
        m_builder->spacing(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += this->event_module::get_output();
    }
    return output;
  }

  bool bspwm_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_MONITOR) {
      builder->node(m_monitors[m_index]->label);
      return true;
    } else if (tag == TAG_LABEL_STATE && !m_monitors[m_index]->workspaces.empty()) {
      size_t workspace_n{0U};

      if (m_scroll) {
        builder->action(mousebtn::SCROLL_DOWN, *this, m_revscroll ? EVENT_NEXT : EVENT_PREV, "");
        builder->action(mousebtn::SCROLL_UP, *this, m_revscroll ? EVENT_PREV : EVENT_NEXT, "");
      }

      for (auto&& ws : m_monitors[m_index]->workspaces) {
        if (ws.second.get()) {
          if (workspace_n != 0 && *m_labelseparator) {
            builder->node(m_labelseparator);
          }

          workspace_n++;

          if (m_click) {
            builder->action(mousebtn::LEFT, *this, EVENT_FOCUS, sstream() << m_index << "+" << workspace_n, ws.second);
          } else {
            builder->node(ws.second);
          }

          if (m_inlinemode && m_monitors[m_index]->focused && check_mask(ws.first, bspwm_state::FOCUSED)) {
            for (auto&& mode : m_monitors[m_index]->modes) {
              builder->node(mode);
            }
          }
        }
      }

      if (m_scroll) {
        builder->action_close();
        builder->action_close();
      }

      return workspace_n > 0;
    } else if (tag == TAG_LABEL_MODE && !m_inlinemode && m_monitors[m_index]->focused &&
               !m_monitors[m_index]->modes.empty()) {
      int modes_n = 0;

      for (auto&& mode : m_monitors[m_index]->modes) {
        if (mode && *mode) {
          builder->node(mode);
          modes_n++;
        }
      }

      return modes_n > 0;
    }

    return false;
  }

  void bspwm_module::action_focus(const string& data) {
    size_t separator{string_util::find_nth(data, 0, "+", 1)};
    size_t monitor_n{std::strtoul(data.substr(0, separator).c_str(), nullptr, 10)};
    string workspace_n{data.substr(separator + 1)};

    if (monitor_n < m_monitors.size()) {
      send_command("desktop -f " + m_monitors[monitor_n]->name + ":^" + workspace_n,
          "Sending desktop focus command to ipc handler");
    } else {
      m_log.err("%s: Invalid monitor index in command: %s", name(), data);
    }
  }
  void bspwm_module::action_next() {
    focus_direction(true);
  }

  void bspwm_module::action_prev() {
    focus_direction(false);
  }

  void bspwm_module::focus_direction(bool next) {
    string scrolldir = next ? "next" : "prev";
    string modifier;

    if (m_occscroll) {
      modifier += ".occupied";
    }

    if (m_pinworkspaces) {
      modifier += ".local";
      for (const auto& mon : m_monitors) {
        if (m_bar.monitor->match(mon->name, false) && !mon->focused) {
          send_command("monitor -f " + mon->name, "Sending monitor focus command to ipc handler");
          break;
        }
      }
    }

    send_command("desktop -f " + scrolldir + modifier, "Sending desktop " + scrolldir + " command to ipc handler");
  }

  void bspwm_module::send_command(const string& payload_cmd, const string& log_info) {
    auto ipc = bspwm_util::make_connection();
    auto payload = bspwm_util::make_payload(payload_cmd);
    m_log.info("%s: %s", name(), log_info);
    ipc->send(payload->data, payload->len, 0);
    ipc->disconnect();
  }
}  // namespace modules

POLYBAR_NS_END
