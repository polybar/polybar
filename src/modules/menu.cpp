#include "modules/menu.hpp"

#include "drawtypes/label.hpp"
#include "events/signal.hpp"
#include "modules/meta/base.inl"
#include "utils/actions.hpp"
#include "utils/scope.hpp"

POLYBAR_NS

namespace modules {
  template class module<menu_module>;

  menu_module::menu_module(const bar_settings& bar, string name_, const config& config)
      : static_module<menu_module>(bar, move(name_), config) {
    m_expand_right = m_conf.get(name(), "expand-right", m_expand_right);

    m_router->register_action_with_data(EVENT_OPEN, [this](const std::string& data) { action_open(data); });
    m_router->register_action(EVENT_CLOSE, [this]() { action_close(); });
    m_router->register_action_with_data(EVENT_EXEC, [this](const std::string& data) { action_exec(data); });

    string default_format;
    if (m_expand_right) {
      default_format += TAG_LABEL_TOGGLE;
      default_format += TAG_MENU;
    } else {
      default_format += TAG_MENU;
      default_format += TAG_LABEL_TOGGLE;
    }

    m_formatter->add(DEFAULT_FORMAT, default_format, {TAG_LABEL_TOGGLE, TAG_MENU});

    if (m_formatter->has(TAG_LABEL_TOGGLE)) {
      m_labelopen = load_label(m_conf, name(), "label-open");
      m_labelclose = load_optional_label(m_conf, name(), "label-close", "x");
    }

    m_labelseparator = load_optional_label(m_conf, name(), "label-separator", "");

    if (!m_formatter->has(TAG_MENU)) {
      return;
    }

    while (true) {
      string level_param{"menu-" + to_string(m_levels.size())};

      if (m_conf.get(name(), level_param + "-0", ""s).empty()) {
        break;
      }

      m_log.trace("%s: Creating menu level %i", name(), m_levels.size());
      m_levels.emplace_back(std::make_unique<menu_tree>());

      while (true) {
        string item_param{level_param + "-" + to_string(m_levels.back()->items.size())};

        if (m_conf.get(name(), item_param, ""s).empty()) {
          break;
        }

        m_log.trace("%s: Creating menu level item %i", name(), m_levels.back()->items.size());
        auto item = std::make_unique<menu_tree_item>();
        item->label = load_label(m_conf, name(), item_param);
        item->exec = m_conf.get(name(), item_param + "-exec", actions_util::get_action_string(*this, EVENT_CLOSE, ""));
        m_levels.back()->items.emplace_back(move(item));
      }
    }
  }

  bool menu_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_TOGGLE && m_level == -1) {
      builder->action(mousebtn::LEFT, *this, string(EVENT_OPEN), "0", m_labelopen);
    } else if (tag == TAG_LABEL_TOGGLE && m_level > -1) {
      builder->action(mousebtn::LEFT, *this, EVENT_CLOSE, "", m_labelclose);
    } else if (tag == TAG_MENU && m_level > -1) {
      auto spacing = m_formatter->get(get_format())->spacing;
      // Insert separator after menu-toggle and before menu-items for expand-right=true
      if (m_expand_right && *m_labelseparator) {
        builder->node(m_labelseparator);
        builder->spacing(spacing);
      }
      auto&& items = m_levels[m_level]->items;
      for (size_t i = 0; i < items.size(); i++) {
        auto&& item = items[i];
        builder->action(
            mousebtn::LEFT, *this, string(EVENT_EXEC), to_string(m_level) + "-" + to_string(i), item->label);
        if (item != m_levels[m_level]->items.back()) {
          builder->spacing(spacing);
          if (*m_labelseparator) {
            builder->node(m_labelseparator);
            builder->spacing(spacing);
          }
          // Insert separator after last menu-item and before menu-toggle for expand-right=false
        } else if (!m_expand_right && *m_labelseparator) {
          builder->spacing(spacing);
          builder->node(m_labelseparator);
        }
      }
    } else {
      return false;
    }
    return true;
  }

  void menu_module::action_open(const string& data) {
    string level = data.empty() ? "0" : data;
    int level_num = m_level = std::strtol(level.c_str(), nullptr, 10);
    m_log.info("%s: Opening menu level '%i'", name(), static_cast<int>(level_num));

    if (static_cast<size_t>(level_num) >= m_levels.size()) {
      m_log.warn("%s: Cannot open unexisting menu level '%s'", name(), level);
      m_level = -1;
    } else {
      m_level = level_num;
    }
    broadcast();
  }

  void menu_module::action_close() {
    m_log.info("%s: Closing menu tree", name());
    if (m_level != -1) {
      m_level = -1;
      broadcast();
    }
  }

  void menu_module::action_exec(const string& element) {
    auto sep = element.find("-");

    if (sep == element.npos) {
      m_log.err("%s: Malformed data for exec action (data: '%s')", name(), element);
    }

    auto level = std::strtoul(element.substr(0, sep).c_str(), nullptr, 10);
    auto item = std::strtoul(element.substr(sep + 1).c_str(), nullptr, 10);

    if (level >= m_levels.size() || item >= m_levels[level]->items.size()) {
      m_log.err("%s: menu-exec-%d-%d doesn't exist (data: '%s')", name(), level, item, element);
    }

    string exec = m_levels[level]->items[item]->exec;
    // Send exec action to be executed
    m_sig.emit(signals::ipc::action{std::move(exec)});

    /*
     * Only close the menu if the executed action is visible in the menu
     * This stops the menu from closing, if the exec action comes from an
     * external source
     */
    if (m_level == (int)level) {
      m_level = -1;
      broadcast();
    }
  }
}  // namespace modules

POLYBAR_NS_END
