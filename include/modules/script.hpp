#pragma once

#include "adapters/script_runner.hpp"
#include "modules/meta/base.hpp"
#include "modules/meta/types.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"

POLYBAR_NS

namespace modules {
  class script_module : public module<script_module> {
   public:
    explicit script_module(const bar_settings&, string, const config&);

    void start() override;
    void stop() override;

    string get_output();
    string get_format() const;

    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = SCRIPT_TYPE;

   protected:
    bool check_condition();

   private:
    void handle_runner_update(const script_runner::data&);

    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_LABEL_FAIL = "<label-fail>";
    static constexpr auto FORMAT_FAIL = "format-fail";

    const bool m_tail;
    const script_runner::interval m_interval_success{0};
    const script_runner::interval m_interval_fail{0};
    const script_runner::interval m_interval_if{0};

    script_runner m_runner;

    map<mousebtn, string> m_actions;

    label_t m_label;
    label_t m_label_fail;

    int m_exit_status{0};
    script_runner::data m_data;
    std::mutex m_data_mutex;
  };
} // namespace modules

POLYBAR_NS_END
