#pragma once

#include <chrono>

#include "modules/meta/event_module.hpp"
#include "utils/command.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

#define OUTPUT_ACTION(BUTTON)     \
  if (!m_actions[BUTTON].empty()) \
  m_builder->cmd(BUTTON, string_util::replace_all(m_actions[BUTTON], "%counter%", counter_str))

namespace modules {
  class script_module : public event_module<script_module> {
   public:
    using event_module::event_module;

    void setup();
    void stop();
    void idle();
    bool has_event();
    bool update();
    string get_output();
    bool build(builder* builder, string tag) const;

   protected:
    static constexpr auto TAG_OUTPUT = "<output>";

    command_util::command_t m_command;

    string m_exec;
    bool m_tail = false;
    chrono::duration<double> m_interval{0};
    size_t m_maxlen = 0;
    bool m_ellipsis = true;
    map<mousebtn, string> m_actions;

    string m_output;
    string m_prev;
    int m_counter{0};
  };
}

POLYBAR_NS_END
