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

    void setup();
    bool build(builder* builder, string tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

   private:
    static constexpr auto TAG_LABEL_TOGGLE = "<label-toggle>";
    static constexpr auto TAG_MENU = "<menu>";

    static constexpr auto EVENT_MENU_OPEN = "menu-open-";
    static constexpr auto EVENT_MENU_CLOSE = "menu-close";

    label_t m_labelopen;
    label_t m_labelclose;
    label_t m_labelseparator;

    vector<unique_ptr<menu_tree>> m_levels;

    std::atomic<int> m_level{-1};
  };
}

LEMONBUDDY_NS_END
