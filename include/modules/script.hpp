#pragma once

#include <atomic>
#include <mutex>

#include "adapters/script_runner.hpp"
#include "modules/meta/base.hpp"
#include "utils/command.hpp"
#include "utils/io.hpp"

POLYBAR_NS

namespace modules {
  class script_module : public module<script_module> {
   public:
    using interval = chrono::duration<double>;
    explicit script_module(const bar_settings&, string);

    void start() override;
    void stop() override;

    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "custom/script";

   protected:
    interval process();
    bool check_condition();

    interval run();
    interval run_tail();

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    const bool m_tail;
    const interval m_interval{0};
    const vector<pair<string, string>> m_env;

    script_runner m_runner;

    map<mousebtn, string> m_actions;

    label_t m_label;
    std::atomic_bool m_stopping{false};
  };
}  // namespace modules

POLYBAR_NS_END
