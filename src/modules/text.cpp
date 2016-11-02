#include "modules/text.hpp"

LEMONBUDDY_NS

namespace modules {
  void text_module::setup() {
    m_formatter->add("content", "", {});

    if (m_formatter->get("content")->value.empty())
      throw module_error(name() + ".content is empty or undefined");

    m_formatter->get("content")->value =
        string_util::replace_all(m_formatter->get("content")->value, " ", BUILDER_SPACE_TOKEN);
  }

  string text_module::get_format() const {
    return "content";
  }

  string text_module::get_output() {
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
}

LEMONBUDDY_NS_END
