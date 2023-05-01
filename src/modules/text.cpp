#include "modules/text.hpp"

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<text_module>;

  text_module::text_module(const bar_settings& bar, string name_, const config& config)
      : static_module<text_module>(bar, move(name_), config) {
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});
    m_formatter->add_optional("content", {});

    if (m_formatter->has_format("content")) {
      m_conf.warn_deprecated(name(), "content", "format");

      if (m_formatter->get("content")->value.empty()) {
        throw module_error(name() + ".content is empty or undefined");
      }

      m_format = "content";
    } else {
      m_format = DEFAULT_FORMAT;

      if (m_formatter->has(TAG_LABEL, DEFAULT_FORMAT)) {
        m_label = load_label(m_conf, name(), TAG_LABEL);
      }
    }
  }

  string text_module::get_format() const {
    return m_format;
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

  bool text_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }

} // namespace modules

POLYBAR_NS_END
