#include "modules/script.hpp"
#include "drawtypes/label.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<script_module>;

  script_module::script_module(const bar_settings& bar, string name_) : module<script_module>(bar, move(name_)) {
    m_exec = m_conf.get(name(), "exec", m_exec);
    m_exec_if = m_conf.get(name(), "exec-if", m_exec_if);
    m_maxlen = m_conf.get(name(), "maxlen", m_maxlen);
    m_ellipsis = m_conf.get(name(), "ellipsis", m_ellipsis);
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 5s);

    m_conf.warn_deprecated(
        name(), "maxlen", "\"format = <label>\" and \"label = %output:0:" + to_string(m_maxlen) + "%\"");

    m_actions[mousebtn::LEFT] = m_conf.get(name(), "click-left", ""s);
    m_actions[mousebtn::MIDDLE] = m_conf.get(name(), "click-middle", ""s);
    m_actions[mousebtn::RIGHT] = m_conf.get(name(), "click-right", ""s);
    m_actions[mousebtn::SCROLL_UP] = m_conf.get(name(), "scroll-up", ""s);
    m_actions[mousebtn::SCROLL_DOWN] = m_conf.get(name(), "scroll-down", ""s);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_OUTPUT, TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), "label", "%output%");
    } else if (m_formatter->has(TAG_OUTPUT)) {
      m_log.warn("%s: The format tag <output> is deprecated, use <label> instead", name());
    }
  }

  void script_module::start() {
    m_mainthread = thread([this] {
      try {
        while (running() && !m_stopping) {
          std::unique_lock<mutex> guard(m_updatelock);

          // Execute the condition command if specified
          if (!m_exec_if.empty() && command_util::make_command(m_exec_if)->exec(true) != 0) {
            if (!m_output.empty()) {
              broadcast();
              m_output.clear();
              m_prev.clear();
            }

            if (m_interval >= 1s) {
              sleep(m_interval);
            } else {
              sleep(1s);
            }
          } else {
            this->process();
            this->sleep(this->sleep_duration());
          }
        }
      } catch (const exception& err) {
        halt(err.what());
      }
    });
  }

  void script_module::stop() {
    std::lock_guard<mutex> guard(m_updatelock, std::adopt_lock);

    m_stopping = true;
    this->wakeup();

    if (m_command) {
      if (m_command->is_running()) {
        m_log.warn("%s: Stopping shell command", name());
      }
      m_command->terminate();
    }

    this->module::stop();
  }

  string script_module::get_output() {
    if (m_output.empty()) {
      return "";
    }

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%output%", m_output);
    }

    if (m_maxlen > 0 && m_output.length() > m_maxlen) {
      m_output.erase(m_maxlen);
      m_output += m_ellipsis ? "..." : "";
    }

    auto counter_str = to_string(m_counter);
    string output{module::get_output()};

    if (!m_actions[mousebtn::LEFT].empty()) {
      m_builder->cmd(mousebtn::LEFT, string_util::replace_all(m_actions[mousebtn::LEFT], "%counter%", counter_str));
    }
    if (!m_actions[mousebtn::MIDDLE].empty()) {
      m_builder->cmd(mousebtn::MIDDLE, string_util::replace_all(m_actions[mousebtn::MIDDLE], "%counter%", counter_str));
    }
    if (!m_actions[mousebtn::RIGHT].empty()) {
      m_builder->cmd(mousebtn::RIGHT, string_util::replace_all(m_actions[mousebtn::RIGHT], "%counter%", counter_str));
    }
    if (!m_actions[mousebtn::SCROLL_UP].empty()) {
      m_builder->cmd(
          mousebtn::SCROLL_UP, string_util::replace_all(m_actions[mousebtn::SCROLL_UP], "%counter%", counter_str));
    }
    if (!m_actions[mousebtn::SCROLL_DOWN].empty()) {
      m_builder->cmd(
          mousebtn::SCROLL_DOWN, string_util::replace_all(m_actions[mousebtn::SCROLL_DOWN], "%counter%", counter_str));
    }

    m_builder->append(output);

    return m_builder->flush();
  }

  bool script_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_OUTPUT) {
      builder->node(m_output);
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
