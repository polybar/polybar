#pragma once

#include "modules/meta/input_handler.hpp"
#include "modules/meta/static_module.hpp"

POLYBAR_NS

namespace modules {
  class menu_module : public static_module<menu_module>, public input_handler {
   public:
    struct menu_tree_item {
      string exec;
      label_t label;
    };

    struct menu_tree {
      vector<unique_ptr<menu_tree_item>> items;
    };

   public:
    explicit menu_module(const bar_settings&, string);

    bool build(builder* builder, const string& tag) const;
    void update() {}

   protected:
    bool input(string&& cmd);

   private:
    static constexpr auto TAG_LABEL_TOGGLE = "<label-toggle>";
    static constexpr auto TAG_MENU = "<menu>";

    static constexpr auto EVENT_MENU_OPEN = "menu-open-";
    static constexpr auto EVENT_MENU_CLOSE = "menu-close";

    bool m_expand_right{true};

    label_t m_labelopen;
    label_t m_labelclose;
    label_t m_labelseparator;

    vector<unique_ptr<menu_tree>> m_levels;

    std::atomic<int> m_level{-1};
  };
}

POLYBAR_NS_END
