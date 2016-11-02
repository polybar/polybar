#include "modules/menu.hpp"
#include "utils/scope.hpp"

LEMONBUDDY_NS

namespace modules {
  void menu_module::setup() {
    string default_format{TAG_LABEL_TOGGLE + string{" "} + TAG_MENU};

    m_formatter->add(DEFAULT_FORMAT, default_format, {TAG_LABEL_TOGGLE, TAG_MENU});

    if (m_formatter->has(TAG_LABEL_TOGGLE)) {
      m_labelopen = load_label(m_conf, name(), "label-open");
      m_labelclose = load_optional_label(m_conf, name(), "label-close", "x");
    }

    m_labelseparator = load_optional_label(m_conf, name(), "label-separator", "");

    if (!m_formatter->has(TAG_MENU))
      return;

    while (true) {
      string level_param{"menu-" + to_string(m_levels.size())};

      if (m_conf.get<string>(name(), level_param + "-0", "").empty())
        break;

      m_log.trace("%s: Creating menu level %i", name(), m_levels.size());
      m_levels.emplace_back(make_unique<menu_tree>());

      while (true) {
        string item_param{level_param + "-" + to_string(m_levels.back()->items.size())};

        if (m_conf.get<string>(name(), item_param, "").empty())
          break;

        m_log.trace("%s: Creating menu level item %i", name(), m_levels.back()->items.size());
        auto item = make_unique<menu_tree_item>();
        item->label = load_label(m_conf, name(), item_param);
        item->exec = m_conf.get<string>(name(), item_param + "-exec", EVENT_MENU_CLOSE);
        m_levels.back()->items.emplace_back(std::move(item));
      }
    }
  }

  bool menu_module::build(builder* builder, string tag) const {
    if (tag == TAG_LABEL_TOGGLE && m_level == -1) {
      builder->cmd(mousebtn::LEFT, string(EVENT_MENU_OPEN) + "0");
      builder->node(m_labelopen);
      builder->cmd_close(true);
    } else if (tag == TAG_LABEL_TOGGLE && m_level > -1) {
      builder->cmd(mousebtn::LEFT, EVENT_MENU_CLOSE);
      builder->node(m_labelclose);
      builder->cmd_close(true);
    } else if (tag == TAG_MENU && m_level > -1) {
      for (auto&& item : m_levels[m_level]->items) {
        if (item != m_levels[m_level]->items.front())
          builder->space();
        if (*m_labelseparator)
          builder->node(m_labelseparator, true);
        builder->cmd(mousebtn::LEFT, item->exec);
        builder->node(item->label);
        builder->cmd_close(true);
      }
    } else {
      return false;
    }
    return true;
  }

  bool menu_module::handle_event(string cmd) {
    if (cmd.compare(0, 4, "menu") != 0)
      return false;

    // broadcast update when leaving leaving the function
    auto exit_handler = scope_util::make_exit_handler<>([this]() {
      if (!m_threads.empty()) {
        m_log.trace("%s: Cleaning up previous broadcast threads", name());
        for (auto&& thread : m_threads)
          if (thread.joinable())
            thread.join();
        m_threads.clear();
      }

      m_log.trace("%s: Dispatching broadcast thread", name());
      m_threads.emplace_back(thread(&menu_module::broadcast, this));
    });

    if (cmd.compare(0, strlen(EVENT_MENU_OPEN), EVENT_MENU_OPEN) == 0) {
      auto level = cmd.substr(strlen(EVENT_MENU_OPEN));

      if (level.empty())
        level = "0";

      m_level = std::atoi(level.c_str());
      m_log.info("%s: Opening menu level '%i'", name(), static_cast<int>(m_level));

      if (static_cast<size_t>(m_level) >= m_levels.size()) {
        m_log.warn("%s: Cannot open unexisting menu level '%i'", name(), level);
        m_level = -1;
      }
    } else if (cmd == EVENT_MENU_CLOSE) {
      m_log.info("%s: Closing menu tree", name());
      m_level = -1;
    }

    return true;
  }

  bool menu_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END
