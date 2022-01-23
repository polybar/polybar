#include "modules/script.hpp"

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  script_module::script_module(const bar_settings& bar, string name_)
      : module<script_module>(bar, move(name_))
      , m_tail(m_conf.get(name(), "tail", false))
      , m_interval(m_conf.get<script_runner::interval>(name(), "interval", m_tail ? 0s : 5s))
      , m_runner([this]() { broadcast(); }, m_conf.get(name(), "exec", ""s), m_conf.get(name(), "exec-if", ""s), m_tail,
            m_interval, m_conf.get_with_prefix(name(), "env-")) {
    // Load configured click handlers
    m_actions[mousebtn::LEFT] = m_conf.get(name(), "click-left", ""s);
    m_actions[mousebtn::MIDDLE] = m_conf.get(name(), "click-middle", ""s);
    m_actions[mousebtn::RIGHT] = m_conf.get(name(), "click-right", ""s);
    m_actions[mousebtn::DOUBLE_LEFT] = m_conf.get(name(), "double-click-left", ""s);
    m_actions[mousebtn::DOUBLE_MIDDLE] = m_conf.get(name(), "double-click-middle", ""s);
    m_actions[mousebtn::DOUBLE_RIGHT] = m_conf.get(name(), "double-click-right", ""s);
    m_actions[mousebtn::SCROLL_UP] = m_conf.get(name(), "scroll-up", ""s);
    m_actions[mousebtn::SCROLL_DOWN] = m_conf.get(name(), "scroll-down", ""s);

    // Setup formatting
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), "label", "%output%");
    }
  }

  /**
   * Start the module worker
   */
  void script_module::start() {
    this->module::start();
    m_mainthread = thread([&] {
      try {
        while (running()) {
          script_runner::interval sleep_time;
          if (m_runner.check_condition()) {
            sleep_time = m_runner.process();
          } else {
            m_runner.clear_output();
            sleep_time = std::max(m_interval, script_runner::interval(1s));
          }

          if (m_runner.is_stopping()) {
            break;
          }

          sleep(sleep_time);
        }
      } catch (const exception& err) {
        halt(err.what());
      }
    });
  }

  /**
   * Stop the module worker by terminating any running commands
   */
  void script_module::stop() {
    m_runner.stop();
    wakeup();

    module::stop();
  }

  /**
   * Generate module output
   */
  string script_module::get_output() {
    auto script_output = m_runner.get_output();
    if (script_output.empty()) {
      return "";
    }

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%output%", script_output);
    }

    string cnt{to_string(m_runner.get_counter())};
    string output{module::get_output()};

    for (const auto& a : m_actions) {
      auto btn = a.first;
      auto action = a.second;

      if (!action.empty()) {
        auto action_replaced = string_util::replace_all(action, "%counter%", cnt);

        /*
         * The pid token is only for tailed commands.
         * If the command is not specified or running, replacement is unnecessary as well
         */
        int pid = m_runner.get_pid();
        if (pid != -1) {
          action_replaced = string_util::replace_all(action_replaced, "%pid%", to_string(pid));
        }
        m_builder->action(btn, action_replaced);
      }
    }

    m_builder->append(output);

    return m_builder->flush();
  }

  /**
   * Output format tags
   */
  bool script_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}  // namespace modules

POLYBAR_NS_END
