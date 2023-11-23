#pragma once

#include "modules/meta/timer_module.hpp"
#include "modules/meta/types.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  enum class memtype { NONE = 0, TOTAL, USED, FREE, SHARED, BUFFERS, CACHE, AVAILABLE };
  enum class memory_state { NORMAL = 0, WARN };
  class memory_module : public timer_module<memory_module> {
   public:
    explicit memory_module(const bar_settings&, string, const config&);

    bool update();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = MEMORY_TYPE;

   private:
#define DEF_LABEL "label"
#define DEF_LABEL_WARN "label-warn"
#define DEF_BAR_USED "bar-used"
#define DEF_BAR_FREE "bar-free"
#define DEF_RAMP_USED "ramp-used"
#define DEF_RAMP_FREE "ramp-free"
#define DEF_BAR_SWAP_USED "bar-swap-used"
#define DEF_BAR_SWAP_FREE "bar-swap-free"
#define DEF_RAMP_SWAP_USED "ramp-swap-used"
#define DEF_RAMP_SWAP_FREE "ramp-swap-free"

    static constexpr const char* NAME_LABEL{DEF_LABEL};
    static constexpr const char* NAME_LABEL_WARN{DEF_LABEL_WARN};
    static constexpr const char* NAME_BAR_USED{DEF_BAR_USED};
    static constexpr const char* NAME_BAR_FREE{DEF_BAR_FREE};
    static constexpr const char* NAME_RAMP_USED{DEF_RAMP_USED};
    static constexpr const char* NAME_RAMP_FREE{DEF_RAMP_FREE};
    static constexpr const char* NAME_BAR_SWAP_USED{DEF_BAR_SWAP_USED};
    static constexpr const char* NAME_BAR_SWAP_FREE{DEF_BAR_SWAP_FREE};
    static constexpr const char* NAME_RAMP_SWAP_USED{DEF_RAMP_SWAP_USED};
    static constexpr const char* NAME_RAMP_SWAP_FREE{DEF_RAMP_SWAP_FREE};

    static constexpr const char* TAG_LABEL{"<" DEF_LABEL ">"};
    static constexpr const char* TAG_LABEL_WARN{"<" DEF_LABEL_WARN ">"};
    static constexpr const char* TAG_BAR_USED{"<" DEF_BAR_USED ">"};
    static constexpr const char* TAG_BAR_FREE{"<" DEF_BAR_FREE ">"};
    static constexpr const char* TAG_RAMP_USED{"<" DEF_RAMP_USED ">"};
    static constexpr const char* TAG_RAMP_FREE{"<" DEF_RAMP_FREE ">"};
    static constexpr const char* TAG_BAR_SWAP_USED{"<" DEF_BAR_SWAP_USED ">"};
    static constexpr const char* TAG_BAR_SWAP_FREE{"<" DEF_BAR_SWAP_FREE ">"};
    static constexpr const char* TAG_RAMP_SWAP_USED{"<" DEF_RAMP_SWAP_USED ">"};
    static constexpr const char* TAG_RAMP_SWAP_FREE{"<" DEF_RAMP_SWAP_FREE ">"};
#undef DEF_LABEL
#undef DEF_LABEL_WARN
#undef DEF_BAR_USED
#undef DEF_BAR_FREE
#undef DEF_RAMP_USED
#undef DEF_RAMP_FREE
#undef DEF_BAR_SWAP_USED
#undef DEF_BAR_SWAP_FREE
#undef DEF_RAMP_SWAP_USED
#undef DEF_RAMP_SWAP_FREE

    static constexpr const char* FORMAT_WARN{"format-warn"};

    label_t m_label;
    label_t m_labelwarn;
    progressbar_t m_bar_memused;
    progressbar_t m_bar_memfree;
    int m_perc_memused{0};
    int m_perc_memfree{0};
    int m_perc_memused_warn{90};
    ramp_t m_ramp_memused;
    ramp_t m_ramp_memfree;
    progressbar_t m_bar_swapused;
    progressbar_t m_bar_swapfree;
    int m_perc_swap_used{0};
    int m_perc_swap_free{0};
    ramp_t m_ramp_swapused;
    ramp_t m_ramp_swapfree;
  };
}  // namespace modules

POLYBAR_NS_END
