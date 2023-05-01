#pragma once

#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"

POLYBAR_NS

namespace modules {
  class menu_module : public static_module<menu_module> {
   public:
    struct menu_tree_item {
      string exec;
      label_t label;
    };

    struct menu_tree {
      vector<unique_ptr<menu_tree_item>> items;
    };

   public:
    explicit menu_module(const bar_settings&, string, const config&);

    bool build(builder* builder, const string& tag) const;
    void update() {}

    static constexpr auto TYPE = MENU_TYPE;

    static constexpr auto EVENT_OPEN = "open";
    static constexpr auto EVENT_CLOSE = "close";
    static constexpr auto EVENT_EXEC = "exec";

   protected:
    void action_open(const string& data);
    void action_close();
    void action_exec(const string& item);

   private:
    static constexpr auto TAG_LABEL_TOGGLE = "<label-toggle>";
    static constexpr auto TAG_MENU = "<menu>";

    bool m_expand_right{true};

    label_t m_labelopen;
    label_t m_labelclose;
    label_t m_labelseparator;

    vector<unique_ptr<menu_tree>> m_levels;

    int m_level{-1};
  };
}  // namespace modules

POLYBAR_NS_END
