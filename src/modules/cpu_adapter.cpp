#include "modules/cpu_adapter.hpp"

#include <fstream>

#include "settings.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace modules {

  cpu_adapter::cpu_adapter() : m_log(logger::make()) {}

  void cpu_adapter::start() {
    // Warmup CPU times
    m_cputimes_prev = read_values();

    m_state = cpu_state::RUNNING;

    update();
  }

  void cpu_adapter::stop() {
    m_state = cpu_state::STOPPED;
    m_cputimes_prev.clear();
    m_load.clear();
  }

  bool cpu_adapter::update() {
    if (m_state != cpu_state::RUNNING) {
      return false;
    }

    auto cputimes = read_values();

    // TODO what if read_values fails and sets the ERROR state

    if (cputimes.empty()) {
      return false;
    }

    auto n = cputimes.size();

    m_load.resize(n, 0);

    /*
     * If the number of cores change between updates the current or the previous
     * cpu_times vector may have different lengths, so we only iterate until the
     * end of the shorter one.
     */
    auto m = std::min(n, m_cputimes_prev.size());

    for (size_t i = 0; i < m; i++) {
      auto prev_idle = m_cputimes_prev[i]->idle;
      auto prev_total = m_cputimes_prev[i]->total;

      auto idle = cputimes[i]->idle;
      auto total = cputimes[i]->total;

      auto idle_diff = idle - prev_idle;
      auto total_diff = total - prev_total;

      float load;
      if (total_diff == 0) {
        /*
         * The total cputimes have not changed since the last update, so we
         * return 0 because we would get -inf load otherwise.
         * This generally shouldn't happen because the times in /proc/stat
         * should be monotonically increasing.
         */
        load = 0.0f;
      } else {
        load = 1.0f - static_cast<float>(idle_diff) / static_cast<float>(total_diff);
      }

      m_load[i] = load;
    }

    m_cputimes_prev.swap(cputimes);
    cputimes.clear();
    return true;
  }

  cpu_state cpu_adapter::get_state() const {
    return m_state;
  }

  vector<float> cpu_adapter::get_load() const {
    return m_load;
  }

  int cpu_adapter::num_cores() const {
    return m_load.size();
  }

  vector<cpu_time_t> cpu_adapter::read_values() {
    vector<cpu_time_t> cputimes;
    try {
      std::ifstream in(PATH_CPU_INFO);
      string str;

      while (std::getline(in, str) && str.compare(0, 3, "cpu") == 0) {
        // skip line with accumulated value
        if (str.compare(0, 4, "cpu ") == 0) {
          continue;
        }

        auto values = string_util::split(str, ' ');

        cpu_time_t cputime = std::make_unique<cpu_time>();

        cputime->user = std::stoull(values[1], nullptr, 10);
        cputime->nice = std::stoull(values[2], nullptr, 10);
        cputime->system = std::stoull(values[3], nullptr, 10);
        cputime->idle = std::stoull(values[4], nullptr, 10);
        cputime->iowait = std::stoull(values[5], nullptr, 10);
        cputime->irq = std::stoull(values[6], nullptr, 10);
        cputime->softirq = std::stoull(values[7], nullptr, 10);
        cputime->steal = std::stoull(values[8], nullptr, 10);
        cputime->guest = std::stoull(values[9], nullptr, 10);

        cputime->total = cputime->user + cputime->nice + cputime->system + cputime->idle + cputime->iowait +
                         cputime->irq + cputime->softirq + cputime->steal + cputime->guest;

        cputimes.push_back(std::move(cputime));
      }
    } catch (const std::ios_base::failure& e) {
      // TODO put into error state
      m_log.err("Failed to read CPU values (what: %s)", e.what());
      cputimes.clear();
    }

    return cputimes;
  }

}  // namespace modules

POLYBAR_NS_END
