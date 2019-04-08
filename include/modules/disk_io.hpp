#pragma once

#include <chrono>
#include "modules/meta/timer_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  class disk_io_module : public timer_module<disk_io_module> {
    public:
      explicit disk_io_module(const bar_settings&, string);

      bool update();
      bool build(builder* builder, const string& tag) const;

    private:
      std::vector<std::string> get_disk_names(void);
      std::pair<unsigned long long, unsigned long long> get_disk_read_write(std::string);
      void calculate_disk_io_speeds(std::string);
      float get_time_delta(void);

      const static unsigned READ_SECTORS_OFFSET = 3 - 1;
      const static unsigned WRITE_SECTORS_OFFSET = 7 - 1;

      static constexpr const char* TAG_LABEL{"<label>"};
      static constexpr const char* TAG_ICON_READ{"<indicator-read>"};
      static constexpr const char* TAG_ICON_WRITE{"<indicator-write>"};

      label_t m_label;
      std::chrono::milliseconds m_ms{0};
      float m_time_delta{0.0};

      std::vector<std::string> m_disk_names;
      std::map<std::string, float> m_read_speeds;
      std::map<std::string, unsigned long long> m_read_total;
      std::map<std::string, float> m_write_speeds;
      std::map<std::string, unsigned long long> m_write_total;

      float m_max_write_speed{0};
      float m_max_read_speed{0};

      iconset_t m_icons;

      std::string m_toggle_on_color;
      std::string m_toggle_off_color;
  };
}

POLYBAR_NS_END
