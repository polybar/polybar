#pragma once

#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

namespace modules {
  enum class cpu_state { STOPPED = 0, RUNNING, ERROR };

  struct cpu_time {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t total;
  };

  using cpu_time_t = unique_ptr<cpu_time>;

  class cpu_adapter {
   public:
    cpu_adapter();
    /**
     * Transitions the adapter from the STOPPED to the RUNNING state
     * Can transition to ERROR as well
     */
    void start();
    /**
     * Transitions from RUNNING or ERROR to STOPPED
     */
    void stop();
    /**
     * Reads new load values
     *
     * Must be in the RUNNING state
     */
    bool update();
    cpu_state get_state() const;

    /**
     * Returns the load for each core as a number between 0 and 1
     *
     * Must be in the RUNNING state
     */
    vector<float> get_load() const;

    /**
     * Returns the number of cores in the current measurement
     *
     * Must be in the RUNNING state
     */
    int num_cores() const;

   protected:
    vector<cpu_time_t> read_values();

   private:
    const logger& m_log;

    cpu_state m_state{cpu_state::STOPPED};

    vector<cpu_time_t> m_cputimes_prev;
    vector<float> m_load;
  };

  using cpu_adapter_t = unique_ptr<cpu_adapter>;
}  // namespace modules

POLYBAR_NS_END
