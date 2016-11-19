#pragma once

#include "modules/meta.hpp"
#include "utils/command.hpp"

POLYBAR_NS

struct ipc_hook;  // fwd

namespace modules {
  /**
   * Hook structure that will be fired
   * when receiving a message with specified id
   */
  struct hook {
    string payload;
    string command;
  };

  /**
   * Module that allow users to configure hooks on
   * received ipc messages. The hook will execute the defined
   * shell script and the resulting output will be used
   * as the module content.
   */
  class ipc_module : public static_module<ipc_module> {
   public:
    using static_module::static_module;

    void setup();
    string get_output();
    bool build(builder* builder, string tag) const;
    void on_message(const ipc_hook& msg);

   private:
    static constexpr auto TAG_OUTPUT = "<output>";
    vector<unique_ptr<hook>> m_hooks;
    string m_output;

    map<mousebtn, string> m_actions;
  };
}

POLYBAR_NS_END
