#pragma once

#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    using static_module::static_module;

    void setup() {
      m_formatter->add(FORMAT, "", {});
      if (m_formatter->get(FORMAT)->value.empty())
        throw undefined_format(FORMAT);
    }

    string get_format() {
      return FORMAT;
    }

    string get_output() {
      auto click_left = m_conf.get<string>(name(), "click-left", "");
      auto click_middle = m_conf.get<string>(name(), "click-middle", "");
      auto click_right = m_conf.get<string>(name(), "click-right", "");
      auto scroll_up = m_conf.get<string>(name(), "scroll-up", "");
      auto scroll_down = m_conf.get<string>(name(), "scroll-down", "");

      if (!click_left.empty())
        m_builder->cmd(mousebtn::LEFT, click_left);
      if (!click_middle.empty())
        m_builder->cmd(mousebtn::MIDDLE, click_middle);
      if (!click_right.empty())
        m_builder->cmd(mousebtn::RIGHT, click_right);
      if (!scroll_up.empty())
        m_builder->cmd(mousebtn::SCROLL_UP, scroll_up);
      if (!scroll_down.empty())
        m_builder->cmd(mousebtn::SCROLL_DOWN, scroll_down);

      m_builder->node(module::get_output());

      return m_builder->flush();
    }

   private:
    static constexpr auto FORMAT = "content";
  };
}

LEMONBUDDY_NS_END
