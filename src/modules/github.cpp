#include "modules/github.hpp"

#include <cassert>

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/concurrency.hpp"

POLYBAR_NS

namespace modules {
  template class module<github_module>;

  /**
   * Construct module
   */
  github_module::github_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<github_module>(bar, move(name_), config) {
    config::value module_config = m_conf[config::value::MODULES_ENTRY][name_raw()];

    m_accesstoken = module_config["token"].as<string>();
    m_user = module_config["user"].as<string>(""s);
    m_api_url = module_config["api-url"].as<string>("https://api.github.com/"s);
    if (m_api_url.back() != '/') {
      m_api_url += '/';
    }

    set_interval(60s);
    m_empty_notifications = module_config["empty-notifications"].as<bool>(m_empty_notifications);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "Notifications: %notifications%");
    }

    m_formatter->add(FORMAT_OFFLINE, TAG_LABEL_OFFLINE, {TAG_LABEL_OFFLINE});

    if (m_formatter->has(TAG_LABEL_OFFLINE)) {
      m_label_offline = load_optional_label(m_conf, name(), TAG_LABEL_OFFLINE, "Offline");
    }

    update_label(0);
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
    string content;
    if (m_user.empty()) {
      content = m_http.get(m_api_url + "notifications?access_token=" + m_accesstoken);
    } else {
      content = m_http.get(m_api_url + "notifications", m_user, m_accesstoken);
    }

    long response_code{m_http.response_code()};
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
      if (!m_offline) {
        m_log.info("%s: cannot complete the request to github: %s", name(), e.what());
      }
      m_offline = true;
      return -1;
    }

    m_offline = false;

    size_t pos{0};
    size_t notifications{0};

    while ((pos = content.find("\"unread\":true", pos + 1)) != string::npos) {
      notifications++;
    }

    return notifications;
  }

  string github_module::get_format() const {
    return m_offline ? FORMAT_OFFLINE : DEFAULT_FORMAT;
  }

  void github_module::update_label(const int notifications) {
    if (!m_label) {
      return;
    }

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
    if (tag == TAG_LABEL) {
      builder->node(m_label);
      return true;
    } else if (tag == TAG_LABEL_OFFLINE) {
      builder->node(m_label_offline);
      return true;
    }

    return false;
  }
} // namespace modules

POLYBAR_NS_END
