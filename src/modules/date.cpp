#include "modules/date.hpp"

#include <ctime>
#include <iomanip>

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/env.hpp"

POLYBAR_NS

namespace modules {
  template class module<date_module>;

  date_module::date_module(const bar_settings& bar, string name_) : timer_module<date_module>(bar, move(name_)) {
    if (!m_bar.locale.empty()) {
      datetime_stream.imbue(std::locale(m_bar.locale.c_str()));
    }

    m_dateformat = m_conf.get(name(), "date", ""s);
    m_dateformat_alt = m_conf.get(name(), "date-alt", ""s);
    m_timeformat = m_conf.get(name(), "time", ""s);
    m_timeformat_alt = m_conf.get(name(), "time-alt", ""s);
    m_timezone = m_conf.get(name(), "timezone", ""s);
    m_timezone_alt = m_conf.get(name(), "timezone-alt", m_timezone);

    if (m_dateformat.empty() && m_timeformat.empty()) {
      throw module_error("No date or time format specified");
    }

    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_DATE});
    m_formatter->add(FORMAT_ALT, TAG_LABEL_ALT, {TAG_LABEL_ALT});

    if (m_formatter->has(TAG_DATE)) {
      m_log.warn("%s: The format tag `<date>` is deprecated, use `<label>` instead.", name());

      m_formatter->get(DEFAULT_FORMAT)->value =
          string_util::replace_all(m_formatter->get(DEFAULT_FORMAT)->value, TAG_DATE, TAG_LABEL);
    }

    if (m_formatter->has(TAG_LABEL, DEFAULT_FORMAT)) {
      m_label = load_optional_label(m_conf, name(), "label", "%date%");
    }

    if (m_formatter->has(TAG_LABEL_ALT, FORMAT_ALT)) {
      m_label_alt = load_optional_label(m_conf, name(), "label-alt", "%date-alt%");
    }
  }

  bool date_module::update() {
    auto time = std::time(nullptr);

    string date_string;
    string time_string;

    auto get_date = [this](std::time_t time) -> std::tuple<string, string> {
      auto date_format = m_toggled ? m_dateformat_alt : m_dateformat;
      // Clear stream contents
      datetime_stream.str("");
      datetime_stream.clear();
      datetime_stream << std::put_time(localtime(&time), date_format.c_str());
      auto date_str = datetime_stream.str();

      auto time_format = m_toggled ? m_timeformat_alt : m_timeformat;
      // Clear stream contents
      datetime_stream.str("");
      datetime_stream.clear();
      datetime_stream << std::put_time(localtime(&time), time_format.c_str());
      auto time_str = datetime_stream.str();

      return std::make_tuple(move(date_str), move(time_str));
    };

    string& timezone = m_toggled ? m_timezone_alt : m_timezone;

    auto cleanup = [](bool empty_value) {
      if (!empty_value) {
        tzset();
      }
    };

    {
      env_util::env_guard<decltype(cleanup)> guard("TZ", timezone, cleanup);
      tzset();
      std::tie(date_string, time_string) = get_date(time);
    }

    if (m_date == date_string && m_time == time_string) {
      return false;
    }

    m_date = move(date_string);
    m_time = move(time_string);

    if (!m_toggled && m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%date%", m_date);
      m_label->replace_token("%time%", m_time);
    } else if (m_toggled && m_label_alt) {
      m_label_alt->reset_tokens();
      m_label_alt->replace_token("%date-alt%", m_date);
      m_label_alt->replace_token("%time-alt%", m_time);
    }

    return true;
  }

  bool date_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      if (!m_dateformat_alt.empty() || !m_timeformat_alt.empty()) {
        builder->cmd(mousebtn::LEFT, EVENT_TOGGLE);
        builder->node(m_label);
        builder->cmd_close();
      } else {
        builder->node(m_label);
      }
    } else if (tag == TAG_LABEL_ALT) {
      if (!m_dateformat.empty() || !m_timeformat.empty()) {
        builder->cmd(mousebtn::LEFT, EVENT_TOGGLE);
        builder->node(m_label_alt);
        builder->cmd_close();
      } else {
        builder->node(m_label_alt);
      }
    } else {
      return false;
    }

    return true;
  }

  string date_module::get_format() const {
    if (m_toggled) {
      return FORMAT_ALT;
    } else {
      return DEFAULT_FORMAT;
    }
  }

  bool date_module::input(string&& cmd) {
    if (cmd != EVENT_TOGGLE) {
      return false;
    }
    m_toggled = !m_toggled;
    wakeup();
    return true;
  }
}  // namespace modules

POLYBAR_NS_END
