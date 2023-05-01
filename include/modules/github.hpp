#pragma once

#include <atomic>

#include "modules/meta/timer_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"
#include "utils/http.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Module used to query the GitHub API for notification count
   */
  class github_module : public timer_module<github_module> {
   public:
    explicit github_module(const bar_settings&, string, const config&);

    bool update();
    bool build(builder* builder, const string& tag) const;
    string get_format() const;

    static constexpr auto TYPE = GITHUB_TYPE;

   private:
    void update_label(int);
    int get_number_of_notification();
    string request();
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_LABEL_OFFLINE = "<label-offline>";
    static constexpr auto FORMAT_OFFLINE = "format-offline";

    label_t m_label{};
    label_t m_label_offline{};
    string m_api_url;
    string m_user;
    string m_accesstoken{};
    http_downloader m_http{};
    bool m_empty_notifications{false};
    std::atomic<bool> m_offline{false};
  };
} // namespace modules

POLYBAR_NS_END
