#pragma once

#include "modules/meta/static_module.hpp"
#include "utils/command.hpp"

#include <mutex>


POLYBAR_NS

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

    void start() override;
    void update() {}
    string get_output();
    bool build(builder* builder, const string& tag) const;
    void on_message(const string& message);

    static constexpr auto TYPE = "custom/ipc";

    static constexpr auto EVENT_SEND = "send";

   protected:
    void action_send(const string& data);
    string replace_active_hook_token(string hook_command);

   private:
    static constexpr const char* TAG_OUTPUT{"<output>"};
    size_t get_active();
    void set_active(size_t);
    size_t active_hook;
    mutex active_mutex;
    vector<unique_ptr<hook>> m_hooks;
    map<mousebtn, string> m_actions;
    string m_output;
    size_t m_initial;
  };
}  // namespace modules

POLYBAR_NS_END
