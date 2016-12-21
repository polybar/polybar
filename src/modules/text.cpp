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

    m_formatter->get("content")->value =
        string_util::replace_all(m_formatter->get("content")->value, " ", BUILDER_SPACE_TOKEN);
  }

  string text_module::get_format() const {
    return "content";
  }

  string text_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    auto click_left = m_conf.get<string>(name(), "click-left", "");
    auto click_middle = m_conf.get<string>(name(), "click-middle", "");
    auto click_right = m_conf.get<string>(name(), "click-right", "");
    auto scroll_up = m_conf.get<string>(name(), "scroll-up", "");
    auto scroll_down = m_conf.get<string>(name(), "scroll-down", "");

    if (!click_left.empty()) {
      m_builder->cmd(mousebtn::LEFT, click_left);
    }
    if (!click_middle.empty()) {
      m_builder->cmd(mousebtn::MIDDLE, click_middle);
    }
    if (!click_right.empty()) {
      m_builder->cmd(mousebtn::RIGHT, click_right);
    }
    if (!scroll_up.empty()) {
      m_builder->cmd(mousebtn::SCROLL_UP, scroll_up);
    }
    if (!scroll_down.empty()) {
      m_builder->cmd(mousebtn::SCROLL_DOWN, scroll_down);
    }

    m_builder->append(output);

    return m_builder->flush();
  }
}

POLYBAR_NS_END
