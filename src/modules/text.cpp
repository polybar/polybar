#include "modules/text.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<text_module>;

  text_module::text_module(const bar_settings& bar, string name_) : static_module<text_module>(bar, move(name_)) {
    m_formatter->add("content", "", {});

    if (m_formatter->get("content")->value.empty()) {
      throw module_error(name() + ".content is empty or undefined");
    }
  }

  string text_module::get_format() const {
    return "content";
  }

  string text_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    auto click_left = m_conf.get(name(), "click-left", ""s);
    auto click_middle = m_conf.get(name(), "click-middle", ""s);
    auto click_right = m_conf.get(name(), "click-right", ""s);
    auto scroll_up = m_conf.get(name(), "scroll-up", ""s);
    auto scroll_down = m_conf.get(name(), "scroll-down", ""s);

    if (!click_left.empty()) {
      m_builder->action(mousebtn::LEFT, click_left);
    }
    if (!click_middle.empty()) {
      m_builder->action(mousebtn::MIDDLE, click_middle);
    }
    if (!click_right.empty()) {
      m_builder->action(mousebtn::RIGHT, click_right);
    }
    if (!scroll_up.empty()) {
      m_builder->action(mousebtn::SCROLL_UP, scroll_up);
    }
    if (!scroll_down.empty()) {
      m_builder->action(mousebtn::SCROLL_DOWN, scroll_down);
    }

    m_builder->node(output);

    return m_builder->flush();
  }
} // namespace modules

POLYBAR_NS_END
