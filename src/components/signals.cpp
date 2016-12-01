#include "components/signals.hpp"
#include "components/types.hpp"

POLYBAR_NS

namespace g_signals {
  /**
   * Signals used to communicate with the event loop
   */
  namespace event {
    callback<const eventloop::entry_t&> enqueue{noop<const eventloop::entry_t&>};
    callback<const eventloop::entry_t&> enqueue_delayed{noop<const eventloop::entry_t&>};
  }

  /**
   * Signals used to communicate with the bar window
   */
  namespace bar {
    callback<const string> action_click{noop<const string>};
    callback<const bool> visibility_change{noop<const bool>};
  }

  /**
   * Signals used to communicate with the input parser
   */
  namespace parser {
    callback<const uint32_t> background_change{noop<const uint32_t>};
    callback<const uint32_t> foreground_change{noop<const uint32_t>};
    callback<const uint32_t> underline_change{noop<const uint32_t>};
    callback<const uint32_t> overline_change{noop<const uint32_t>};
    callback<const alignment> alignment_change{noop<const alignment>};
    callback<const attribute> attribute_set{noop<const attribute>};
    callback<const attribute> attribute_unset{noop<const attribute>};
    callback<const attribute> attribute_toggle{noop<const attribute>};
    callback<const int8_t> font_change{noop<const int8_t>};
    callback<const int16_t> pixel_offset{noop<const int16_t>};
    callback<const mousebtn, const string> action_block_open{noop<const mousebtn, const string>};
    callback<const mousebtn> action_block_close{noop<const mousebtn>};
    callback<const uint16_t> ascii_text_write{noop<const uint16_t>};
    callback<const uint16_t> unicode_text_write{noop<const uint16_t>};
    callback<const char*, const size_t> string_write{noop<const char*, const size_t>};
  }

  /**
   * Signals used to communicate with the tray manager
   */
  namespace tray {
    callback<const uint16_t> report_slotcount{noop<const uint16_t>};
  }
}

POLYBAR_NS_END
