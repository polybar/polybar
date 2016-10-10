#pragma once

#include "drawtypes/label.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  struct menu_tree_item {
    string exec;
    label_t label;
  };

  struct menu_tree {
    vector<unique_ptr<menu_tree_item>> items;
  };

  class menu_module : public static_module<menu_module> {
   public:
    using static_module::static_module;

    void setup() {
      auto default_format_string = string{TAG_LABEL_TOGGLE} + " " + string{TAG_MENU};

      m_formatter->add(DEFAULT_FORMAT, default_format_string, {TAG_LABEL_TOGGLE, TAG_MENU});

      if (m_formatter->has(TAG_LABEL_TOGGLE)) {
        m_labelopen = get_config_label(m_conf, name(), "label-open");
        m_labelclose = get_optional_config_label(m_conf, name(), "label-close", "x");
      }

      if (m_formatter->has(TAG_MENU)) {
        int level_n = 0;

        while (true) {
          auto level_path = "menu-" + to_string(level_n);

          if (m_conf.get<string>(name(), level_path + "-0", "") == "")
            break;

          m_levels.emplace_back(make_unique<menu_tree>());

          int item_n = 0;

          while (true) {
            auto item_path = level_path + "-" + to_string(item_n);

            if (m_conf.get<string>(name(), item_path, "") == "")
              break;

            auto item = make_unique<menu_tree_item>();

            item->label = get_config_label(m_conf, name(), item_path);
            item->exec = m_conf.get<string>(name(), item_path + "-exec", EVENT_MENU_CLOSE);

            m_levels.back()->items.emplace_back(std::move(item));

            item_n++;
          }

          level_n++;
        }
      }
    }

    string get_output() {
      m_builder->node(module::get_output());
      return m_builder->flush();
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_LABEL_TOGGLE && m_level == -1) {
        builder->cmd(mousebtn::LEFT, string(EVENT_MENU_OPEN) + "0");
        builder->node(m_labelopen);
        builder->cmd_close(true);
      } else if (tag == TAG_LABEL_TOGGLE && m_level > -1) {
        builder->cmd(mousebtn::LEFT, EVENT_MENU_CLOSE);
        builder->node(m_labelclose);
        builder->cmd_close(true);
      } else if (tag == TAG_MENU && m_level > -1) {
        int i = 0;

        for (auto&& m : m_levels[m_level]->items) {
          if (i++ > 0)
            builder->space();
          builder->color_alpha("77");
          builder->node("/");
          builder->color_close(true);
          builder->space();
          builder->cmd(mousebtn::LEFT, m->exec);
          builder->node(m->label);
          builder->cmd_close(true);
        }
      } else {
        return false;
      }
      return true;
    }

    bool handle_event(string cmd) {
      if (cmd.compare(0, strlen(EVENT_MENU_OPEN), EVENT_MENU_OPEN) == 0) {
        auto level = cmd.substr(strlen(EVENT_MENU_OPEN));

        if (level.empty())
          level = "0";

        m_level = std::atoi(level.c_str());

        if (m_level >= (int)m_levels.size()) {
          m_log.err("%s: Cannot open unexisting menu level '%d'", name(), level);
          m_level = -1;
        }

      } else if (cmd == EVENT_MENU_CLOSE) {
        m_level = -1;
      } else {
        m_level = -1;
        broadcast();
        return false;
      }

      broadcast();

      return true;
    }

    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto TAG_LABEL_TOGGLE = "<label-toggle>";
    static constexpr auto TAG_MENU = "<menu>";

    static constexpr auto EVENT_MENU_OPEN = "menu_open-";
    static constexpr auto EVENT_MENU_CLOSE = "menu_close";

    int m_level = -1;
    vector<unique_ptr<menu_tree>> m_levels;

    label_t m_labelopen;
    label_t m_labelclose;
  };
}

LEMONBUDDY_NS_END
