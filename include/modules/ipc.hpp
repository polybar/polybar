#pragma once

#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
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
    };

   public:
    explicit ipc_module(const bar_settings&, string, const config&);

    void start() override;
    void update();
    string get_output();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;
    void on_message(const string& message);

    static constexpr auto TYPE = IPC_TYPE;

    static constexpr auto EVENT_SEND = "send";
    static constexpr auto EVENT_HOOK = "hook";
    static constexpr auto EVENT_NEXT = "next";
    static constexpr auto EVENT_PREV = "prev";
    static constexpr auto EVENT_RESET = "reset";

   protected:
    void action_send(const string& data);
    void action_hook(const string& data);
    void action_next();
    void action_prev();
    void action_reset();

    void hook_offset(int offset);

    bool has_initial() const;
    bool has_hook() const;

    void set_hook(int h);
    void update_output() ;
   private:
    static constexpr auto TAG_OUTPUT = "<output>";
    static constexpr auto TAG_LABEL = "<label>";

    label_t m_label;

    vector<unique_ptr<hook>> m_hooks;
    map<mousebtn, string> m_actions;
    string m_output;

    int m_initial{-1};
    int m_current_hook{-1};
    void exec_hook();
  };
} // namespace modules

POLYBAR_NS_END
