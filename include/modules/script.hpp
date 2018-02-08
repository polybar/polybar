#pragma once

#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"

POLYBAR_NS

namespace modules {
  class script_module : public module<script_module> {
   public:
    explicit script_module(const bar_settings&, string);
    ~script_module() {}

    void start();
    void stop();

    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    chrono::duration<double> process(const mutex_wrapper<function<chrono::duration<double>()>>& handler) const;
    bool check_condition();

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    mutex_wrapper<function<chrono::duration<double>()>> m_handler;

    unique_ptr<command> m_command;

    bool m_tail;

    string m_exec;
    string m_exec_if;

    chrono::duration<double> m_interval{0};
    map<mousebtn, string> m_actions;

    label_t m_label;
    string m_output;
    string m_prev;
    int m_counter{0};

    bool m_stopping{false};
  };
}

POLYBAR_NS_END
