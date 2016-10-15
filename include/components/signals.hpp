#pragma once

#include <fastdelegate/fastdelegate.hpp>

#include "common.hpp"
#include "components/types.hpp"

LEMONBUDDY_NS

namespace g_signals {
  /**
   * Signals used to communicate with the bar window
   */
  namespace bar {
    static delegate::Signal1<string> action_click;
    static delegate::Signal1<bool> visibility_change;
  }

  /**
   * Signals used to communicate with the input parser
   */
  namespace parser {
    static delegate::Signal1<alignment> alignment_change;
    static delegate::Signal1<attribute> attribute_set;
    static delegate::Signal1<attribute> attribute_unset;
    static delegate::Signal1<attribute> attribute_toggle;
    static delegate::Signal2<mousebtn, string> action_block_open;
    static delegate::Signal1<mousebtn> action_block_close;
    static delegate::Signal2<gc, color> color_change;
    static delegate::Signal1<int> font_change;
    static delegate::Signal1<int> pixel_offset;
    static delegate::Signal1<uint16_t> ascii_text_write;
    static delegate::Signal1<uint16_t> unicode_text_write;
  }

  /**
   * Signals used to communicate with the tray manager
   */
  namespace tray {
    static delegate::Signal1<uint16_t> report_slotcount;
  }
}

LEMONBUDDY_NS_END
