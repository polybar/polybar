#pragma once

#include <condition_variable>
#include <mutex>

#include "modules/meta/base.hpp"
#include "utils/command.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Module that allow users to configure hooks on
   * received ipc messages. The hook will execute the defined
   * shell script and the resulting output will be used
   * as the module content.
   */
  class ipc_module : public module<ipc_module> {
   public:
    /**
     * Hook structure that will be fired
     * when receiving a message with specified id
     */
    struct hook {
      string payload;
      string command;
      size_t interval;
    };

   public:
    explicit ipc_module(const bar_settings&, string);

    void start();
    bool update();
    string get_output();
    bool build(builder* builder, const string& tag) const;
    void on_message(const string& message);

   protected:
    void runner();
    void wakeup();

   private:
    friend class module<ipc_module>;

    static constexpr const char* TAG_OUTPUT{"<output>"};
    vector<unique_ptr<hook>> m_hooks;
    map<mousebtn, string> m_actions;
    string m_output;
    size_t m_initial;

    size_t m_current = 0;
    bool m_should_wake_up = true;
  };
}  // namespace modules

POLYBAR_NS_END
