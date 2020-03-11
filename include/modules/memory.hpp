#pragma once

#include "modules/meta/timer_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  enum class memtype { NONE = 0, TOTAL, USED, FREE, SHARED, BUFFERS, CACHE, AVAILABLE };


  class memory_module : public timer_module<memory_module> {
   public:
    explicit memory_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr const char* TAG_LABEL{"<label>"};
    static constexpr const char* TAG_BAR_AVAILABLE_FREE{"<bar-available-free>"};
    static constexpr const char* TAG_BAR_AVAILABLE_USED{"<bar-available-used>"};
    static constexpr const char* TAG_BAR_FREE{"<bar-free>"};
    static constexpr const char* TAG_BAR_FREE_USED{"<bar-free-used>"};
    static constexpr const char* TAG_RAMP_AVAILABLE_FREE{"<ramp-available-free>"};
    static constexpr const char* TAG_RAMP_AVAILABLE_USED{"<ramp-available-used>"};
    static constexpr const char* TAG_RAMP_FREE{"<ramp-free>"};
    static constexpr const char* TAG_RAMP_FREE_USED{"<ramp-free-used>"};
    static constexpr const char* TAG_BAR_SWAP_FREE{"<bar-swap-free>"};
    static constexpr const char* TAG_BAR_SWAP_USED{"<bar-swap-used>"};
    static constexpr const char* TAG_RAMP_SWAP_FREE{"<ramp-swap-free>"};
    static constexpr const char* TAG_RAMP_SWAP_USED{"<ramp-swap-used>"};

    label_t m_label;
    progressbar_t m_bar_memavail;
    progressbar_t m_bar_memavailused;
    progressbar_t m_bar_memfree;
    progressbar_t m_bar_memfreeused;
    int m_perc_memavail{0};
    int m_perc_memavailused{0};
    int m_perc_memfree{0};
    int m_perc_memfreeused{0};
    ramp_t m_ramp_memavail;
    ramp_t m_ramp_memavailused;
    ramp_t m_ramp_memfree;
    ramp_t m_ramp_memfreeused;
    progressbar_t m_bar_swapfree;
    progressbar_t m_bar_swapused;
    int m_perc_swap_used{0};
    int m_perc_swap_free{0};
    ramp_t m_ramp_swapused;
    ramp_t m_ramp_swapfree;
  };
}

POLYBAR_NS_END
