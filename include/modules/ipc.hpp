#pragma once

#include "modules/meta/static_module.hpp"
#include "utils/command.hpp"

POLYBAR_NS

struct ipc_hook;  // fwd

namespace modules {
  /**
   * Module that allow users to configure hooks on
   * received ipc messages. The hook will execute the defined
   * shell script and the resulting output will be used
   * as the module content.
   */
  class ipc_module : public static_module<ipc_module> {
   public:
    /**
     * Hook structure that will be fired
     * when receiving a message with specified id
     */
    struct hook {
      string payload;
      string command;
    };

   public:
    explicit ipc_module(const bar_settings&, string);

    void start();
    void update() {}
    string get_output();
    bool build(builder* builder, const string& tag) const;
    void on_message(const string& message);

   private:
    static constexpr const char* TAG_OUTPUT{"<output>"};
    vector<unique_ptr<hook>> m_hooks;
    map<mousebtn, string> m_actions;
    string m_output;
    size_t m_initial;
  };
}

POLYBAR_NS_END
