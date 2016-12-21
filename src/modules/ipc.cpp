#include "modules/ipc.hpp"

#include "components/ipc.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/static_module.inl"

POLYBAR_NS

namespace modules {
  template class module<ipc_module>;
  template class static_module<ipc_module>;

  /**
   * Load user-defined ipc hooks and
   * create formatting tags
   */
  ipc_module::ipc_module(const bar_settings& bar, string name_) : static_module<ipc_module>(bar, move(name_)) {
    size_t index = 0;

    for (auto&& command : m_conf.get_list<string>(name(), "hook")) {
      m_hooks.emplace_back(new hook{name() + to_string(++index), command});
    }

    if (m_hooks.empty()) {
      throw module_error("No ipc hooks defined");
    }

    m_actions[mousebtn::LEFT] = m_conf.get<string>(name(), "click-left", "");
    m_actions[mousebtn::MIDDLE] = m_conf.get<string>(name(), "click-middle", "");
    m_actions[mousebtn::RIGHT] = m_conf.get<string>(name(), "click-right", "");
    m_actions[mousebtn::SCROLL_UP] = m_conf.get<string>(name(), "scroll-up", "");
    m_actions[mousebtn::SCROLL_DOWN] = m_conf.get<string>(name(), "scroll-down", "");

    m_formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, {TAG_OUTPUT});
  }

  /**
   * Wrap the output with defined mouse actions
   */
  string ipc_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    if (!m_actions[mousebtn::LEFT].empty()) {
      m_builder->cmd(mousebtn::LEFT, m_actions[mousebtn::LEFT]);
    }
    if (!m_actions[mousebtn::MIDDLE].empty()) {
      m_builder->cmd(mousebtn::MIDDLE, m_actions[mousebtn::MIDDLE]);
    }
    if (!m_actions[mousebtn::RIGHT].empty()) {
      m_builder->cmd(mousebtn::RIGHT, m_actions[mousebtn::RIGHT]);
    }
    if (!m_actions[mousebtn::SCROLL_UP].empty()) {
      m_builder->cmd(mousebtn::SCROLL_UP, m_actions[mousebtn::SCROLL_UP]);
    }
    if (!m_actions[mousebtn::SCROLL_DOWN].empty()) {
      m_builder->cmd(mousebtn::SCROLL_DOWN, m_actions[mousebtn::SCROLL_DOWN]);
    }

    m_builder->append(output);

    return m_builder->flush();
  }

  /**
   * Output content retrieved from hook commands
   */
  bool ipc_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_OUTPUT) {
      builder->node(m_output);
    } else {
      return false;
    }
    return true;
  }

  /**
   * Map received message hook to the ones
   * configured from the user config and
   * execute its command
   */
  void ipc_module::on_message(const ipc_hook& message) {
    bool match = false;

    for (auto&& hook : m_hooks) {
      if (ipc_hook::prefix + hook->payload != message.payload) {
        continue;
      }

      match = true;

      m_log.info("%s: Found matching hook (%s)", name(), hook->payload);
      m_output.clear();

      auto command = command_util::make_command(hook->command);
      command->exec(false);
      command->tail([this](string line) { m_output = line; });
    }

    if (match) {
      broadcast();
    }
  }
}

POLYBAR_NS_END
