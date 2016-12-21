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
    explicit script_module(const bar_settings&, string);

    void stop();
    void idle();
    bool has_event();
    bool update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    static constexpr const char* TAG_OUTPUT{"<output>"};
    static constexpr const char* TAG_LABEL{"<label>"};

    unique_ptr<command> m_command;

    string m_exec;
    bool m_tail{false};
    chrono::duration<double> m_interval{0};
    map<mousebtn, string> m_actions;

    label_t m_label;
    string m_output;
    string m_prev;
    int m_counter{0};

    // @deprecated
    size_t m_maxlen{0};
    // @deprecated
    bool m_ellipsis{true};
  };
}

POLYBAR_NS_END
