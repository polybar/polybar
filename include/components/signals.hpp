#pragma once

#include "common.hpp"

#include "utils/functional.hpp"

POLYBAR_NS

// fwd decl {{{

enum class mousebtn;
enum class syntaxtag;
enum class alignment;
enum class attribute;
enum class gc;
class color;

// }}}

/**
 * @TODO: Allow multiple signal handlers
 */
namespace g_signals {
  namespace bar {
    extern callback<string> action_click;
    extern callback<bool> visibility_change;
  }

  namespace parser {
    extern callback<alignment> alignment_change;
    extern callback<attribute> attribute_set;
    extern callback<attribute> attribute_unset;
    extern callback<attribute> attribute_toggle;
    extern callback<mousebtn, string> action_block_open;
    extern callback<mousebtn> action_block_close;
    extern callback<gc, color> color_change;
    extern callback<int> font_change;
    extern callback<int> pixel_offset;
    extern callback<uint16_t> ascii_text_write;
    extern callback<uint16_t> unicode_text_write;
    extern callback<const char*, size_t> string_write;
  }

  namespace tray {
    extern callback<uint16_t> report_slotcount;
    extern callback<uint32_t> clear_bg;
  }
}

POLYBAR_NS_END
