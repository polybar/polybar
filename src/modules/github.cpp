#include <cassert>

#include "drawtypes/label.hpp"
#include "modules/github.hpp"
#include "utils/concurrency.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<github_module>;

  /**
   * Construct module
   */
  github_module::github_module(const bar_settings& bar, string name_)
      : timer_module<github_module>(bar, move(name_)), m_http(http_util::make_downloader()) {
    m_accesstoken = m_conf.get(name(), "token");
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 60s);
    m_empty_notifications = m_conf.get(name(), "empty-notifications", m_empty_notifications);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "Notifications: %notifications%");

      update_label(0);
    }
  }

  /**
   * Update module contents
   */
  bool github_module::update() {
    auto notification = get_number_of_notification();

    update_label(notification);

    return true;
  }

  string github_module::request() {
    string content{m_http->get("https://api.github.com/notifications?access_token=" + m_accesstoken)};

    long response_code{m_http->response_code()};
    switch (response_code) {
      case 200:
        break;
      case 401:
        throw module_error("Bad credentials");
      case 403:
        throw module_error("Maximum number of login attempts exceeded");
      default:
        throw module_error("Unspecified error (" + to_string(response_code) + ")");
    }

    return content;
  }

  int github_module::get_number_of_notification() {
    string content;
    try {
      content = request();
    } catch (application_error& e) {
      m_log.warn("%s: cannot complete the request to github: %s", name(), e.what());
      return -1;
    }

    size_t pos{0};
    size_t notifications{0};

    while ((pos = content.find("\"unread\":true", pos + 1)) != string::npos) {
      notifications++;
    }

    return notifications;
  }

  void github_module::update_label(const int notifications) {
    if (0 != notifications || m_empty_notifications) {
      m_label->reset_tokens();
      m_label->replace_token("%notifications%", to_string(notifications));
    } else {
      m_label->clear();
    }
  }

  /**
   * Build module content
   */
  bool github_module::build(builder* builder, const string& tag) const {
    if (tag != TAG_LABEL)
      return false;

    builder->node(m_label);
    return true;
  }
}

POLYBAR_NS_END
