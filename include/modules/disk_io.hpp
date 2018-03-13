#pragma once

#include <chrono>
#include "modules/meta/timer_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  /* enum class memtype { NONE = 0, TOTAL, READ, WRITE, SHARED, BUFFERS, CACHE, AVAILABLE }; */

  class disk_io_module : public timer_module<disk_io_module> {
    public:
      explicit disk_io_module(const bar_settings&, string);

      bool update();
      bool build(builder* builder, const string& tag) const;

    private:
      std::vector<std::string> _get_disk_names(void);
      std::pair<unsigned long long, unsigned long long> _get_disk_read_write(std::string);
      void _calculate_disk_io_speeds(std::string);
      float _get_time_delta(void);

      const static unsigned READ_SECTORS_OFFSET = 3 - 1;
      const static unsigned WRITE_SECTORS_OFFSET = 7 - 1;

      static constexpr const char* TAG_LABEL{"<label>"};
      /* static constexpr const char* TAG_BAR_READ{"<bar-read>"}; */
      /* static constexpr const char* TAG_BAR_WRITE{"<bar-write>"}; */

      label_t m_label;
      std::chrono::milliseconds m_ms{0};
      float m_time_delta{0.0};

      std::vector<std::string> m_disk_names;
      std::map<std::string, float> m_read_speeds;
      std::map<std::string, unsigned long long> m_read_total;
      std::map<std::string, float> m_write_speeds;
      std::map<std::string, unsigned long long> m_write_total;
  };
}

POLYBAR_NS_END
