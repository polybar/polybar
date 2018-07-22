#include "modules/text.hpp"
#include "drawtypes/label.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<text_module>;

  text_module::text_module(const bar_settings& bar, string name_) : static_module<text_module>(bar, move(name_)) {
    m_formatter->add(DEFAULT_FORMAT, TAG_CONTENT, {TAG_CONTENT});
    if (m_formatter->has(TAG_CONTENT, DEFAULT_FORMAT)) {
      m_label = load_label(m_conf, name(), TAG_CONTENT);
    }
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

  bool text_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_CONTENT) {
      builder->node(m_label);
    } else {
      return false;
    }
    return true;
  }

}

POLYBAR_NS_END
