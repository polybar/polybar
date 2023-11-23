#include "modules/text.hpp"

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<text_module>;

  text_module::text_module(const bar_settings& bar, string name_, const config& config)
      : static_module<text_module>(bar, move(name_), config) {
    config::value module_config = m_conf[config::value::MODULES_ENTRY][name_raw()];
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});
    m_formatter->add_optional("content", {});

    if (m_formatter->has_format("content")) {
      module_config.warn_deprecated("content", module_config["format"]);

      if (m_formatter->get("content")->value.empty()) {
        throw module_error(name() + ".content is empty or undefined");
      }

      m_format = "content";
    } else {
      m_format = DEFAULT_FORMAT;

      if (m_formatter->has(TAG_LABEL, DEFAULT_FORMAT)) {
        m_label = load_label(module_config[TAG_LABEL]);
      }
    }
  }

  string text_module::get_format() const {
    return m_format;
  }

  string text_module::get_output() {
    config::value module_config = m_conf[config::value::MODULES_ENTRY][name_raw()];

    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    auto click_left = module_config["click-left"].as<string>(""s);
    auto click_middle = module_config["click-middle"].as<string>(""s);
    auto click_right = module_config["click-right"].as<string>(""s);
    auto scroll_up = module_config["scroll-up"].as<string>(""s);
    auto scroll_down = module_config["scroll-down"].as<string>(""s);

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
