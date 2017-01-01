#pragma once

#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"

POLYBAR_NS

namespace modules {
  class script_module : public module<script_module> {
   public:
    explicit script_module(const bar_settings&, string);
    virtual ~script_module() {}

    virtual void start();
    virtual void stop();

    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    virtual void process() = 0;

    static constexpr const char* TAG_OUTPUT{"<output>"};
    static constexpr const char* TAG_LABEL{"<label>"};

    unique_ptr<command> m_command;

    string m_exec;
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

    bool m_stopping{false};
  };
}

POLYBAR_NS_END
