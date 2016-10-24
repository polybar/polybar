#pragma once

#include "common.hpp"
#include "components/types.hpp"

LEMONBUDDY_NS

/**
 * @TODO: Allow multiple signal handlers
 */
namespace g_signals {
  /**
   * Signals used to communicate with the bar window
   */
  namespace bar {
    static function<void(string)> action_click;
    static function<void(bool)> visibility_change;
  }

  /**
   * Signals used to communicate with the input parser
   */
  namespace parser {
    static function<void(alignment)> alignment_change;
    static function<void(attribute)> attribute_set;
    static function<void(attribute)> attribute_unset;
    static function<void(attribute)> attribute_toggle;
    static function<void(mousebtn, string)> action_block_open;
    static function<void(mousebtn)> action_block_close;
    static function<void(gc, color)> color_change;
    static function<void(int)> font_change;
    static function<void(int)> pixel_offset;
    static function<void(uint16_t)> ascii_text_write;
    static function<void(uint16_t)> unicode_text_write;
  }

  /**
   * Signals used to communicate with the tray manager
   */
  namespace tray {
    static function<void(uint16_t)> report_slotcount;
  }
}

LEMONBUDDY_NS_END
