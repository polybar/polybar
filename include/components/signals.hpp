#pragma once

#include "common.hpp"

#include "utils/functional.hpp"

POLYBAR_NS

// fwd decl {{{

enum class mousebtn : uint8_t;
enum class syntaxtag : uint8_t;
enum class alignment : uint8_t;
enum class attribute : uint8_t;
enum class gc : uint8_t;

// }}}

/**
 * @TODO: Allow multiple signal handlers
 */
namespace g_signals {
  namespace bar {
    extern callback<string> action_click;
    extern callback<const bool> visibility_change;
  }

  namespace parser {
    extern callback<const alignment> alignment_change;
    extern callback<const attribute> attribute_set;
    extern callback<const attribute> attribute_unset;
    extern callback<const mousebtn, string> action_block_open;
    extern callback<const mousebtn> action_block_close;
    extern callback<const gc, const uint32_t> color_change;
    extern callback<const int8_t> font_change;
    extern callback<const int16_t> pixel_offset;
    extern callback<const uint16_t> ascii_text_write;
    extern callback<const uint16_t> unicode_text_write;
    extern callback<const char*, const size_t> string_write;
  }

  namespace tray {
    extern callback<const uint16_t> report_slotcount;
    extern callback<const uint32_t> clear_bg;
  }
}

POLYBAR_NS_END
