#include "modules/menu.hpp"

#include "drawtypes/label.hpp"
#include "utils/factory.hpp"
#include "utils/scope.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<menu_module>;

  menu_module::menu_module(const bar_settings& bar, string name_) : static_module<menu_module>(bar, move(name_)) {
    m_expand_right = m_conf.get(name(), "expand-right", m_expand_right);

    string default_format;
    if(m_expand_right) {
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
      m_levels.emplace_back(factory_util::unique<menu_tree>());

      while (true) {
        string item_param{level_param + "-" + to_string(m_levels.back()->items.size())};

        if (m_conf.get(name(), item_param, ""s).empty()) {
          break;
        }

        m_log.trace("%s: Creating menu level item %i", name(), m_levels.back()->items.size());
        auto item = factory_util::unique<menu_tree_item>();
        item->label = load_label(m_conf, name(), item_param);
        item->exec = m_conf.get(name(), item_param + "-exec", string{EVENT_MENU_CLOSE});
        m_levels.back()->items.emplace_back(move(item));
      }
    }
  }

  bool menu_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_TOGGLE && m_level == -1) {
      builder->cmd(mousebtn::LEFT, string(EVENT_MENU_OPEN) + "0");
      builder->node(m_labelopen);
      builder->cmd_close();
    } else if (tag == TAG_LABEL_TOGGLE && m_level > -1) {
      builder->cmd(mousebtn::LEFT, EVENT_MENU_CLOSE);
      builder->node(m_labelclose);
      builder->cmd_close();
    } else if (tag == TAG_MENU && m_level > -1) {
      auto spacing = m_formatter->get(get_format())->spacing;
      for (auto&& item : m_levels[m_level]->items) {
        /*
         * Depending on whether the menu items are to the left or right of the toggle label, the items need to be
         * drawn before or after the spacings and the separator
         *
         * If the menu expands to the left, the separator should be drawn on the right side because otherwise the menu
         * would look like this:
         * | x | y <label-toggle>
         */
        if(!m_expand_right) {
          builder->cmd(mousebtn::LEFT, item->exec);
          builder->node(item->label);
          builder->cmd_close();
        }

        if (*m_labelseparator) {
          if (item != m_levels[m_level]->items[0]) {
            builder->space(spacing);
          }
          builder->node(m_labelseparator);
          builder->space(spacing);
        }

        /*
         * If the menu expands to the right, the separator should be drawn on the left side because otherwise the menu
         * would look like this:
         * <label-toggle> x | y |
         */
        if(m_expand_right) {
          builder->cmd(mousebtn::LEFT, item->exec);
          builder->node(item->label);
          builder->cmd_close();
        }
      }
    } else {
      return false;
    }
    return true;
  }

  bool menu_module::input(string&& cmd) {
    if (cmd.compare(0, 4, "menu") != 0 && m_level > -1) {
      for (auto&& item : m_levels[m_level]->items) {
        if (item->exec == cmd) {
          m_level = -1;
          break;
        }
      }
      broadcast();
      return false;
    }

    if (cmd.compare(0, strlen(EVENT_MENU_OPEN), EVENT_MENU_OPEN) == 0) {
      auto level = cmd.substr(strlen(EVENT_MENU_OPEN));

      if (level.empty()) {
        level = "0";
      }
      m_level = std::strtol(level.c_str(), nullptr, 10);
      m_log.info("%s: Opening menu level '%i'", name(), static_cast<int>(m_level));

      if (static_cast<size_t>(m_level) >= m_levels.size()) {
        m_log.warn("%s: Cannot open unexisting menu level '%i'", name(), level);
        m_level = -1;
      }
    } else if (cmd == EVENT_MENU_CLOSE) {
      m_log.info("%s: Closing menu tree", name());
      m_level = -1;
    } else {
      return false;
    }

    broadcast();
    return true;
  }
}

POLYBAR_NS_END
