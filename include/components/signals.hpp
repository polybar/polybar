#pragma once

#include "common.hpp"

#include "components/eventloop.hpp"
#include "utils/functional.hpp"

POLYBAR_NS

// fwd decl {{{

enum class mousebtn : uint8_t;
enum class syntaxtag : uint8_t;
enum class alignment : uint8_t;
enum class attribute : uint8_t;

// }}}

/**
 * @TODO: Allow multiple signal handlers
 * @TODO: Encapsulate signals
 */
namespace g_signals {
  /**
   * Helper used to create no-op "callbacks"
   */
  template <typename... T>
  static callback<T...> noop = [](T...) {};

  /**
   * Signals used to communicate with the event loop
   */
  namespace event {
    extern callback<const eventloop::entry_t&> enqueue;
    extern callback<const eventloop::entry_t&> enqueue_delayed;
  }

  /**
   * Signals used to communicate with the bar window
   */
  namespace bar {
    extern callback<string> action_click;
    extern callback<const bool> visibility_change;
  }

  /**
   * Signals used to communicate with the input parser
   */
  namespace parser {
    extern callback<const uint32_t> background_change;
    extern callback<const uint32_t> foreground_change;
    extern callback<const uint32_t> underline_change;
    extern callback<const uint32_t> overline_change;
    extern callback<const alignment> alignment_change;
    extern callback<const attribute> attribute_set;
    extern callback<const attribute> attribute_unset;
    extern callback<const attribute> attribute_toggle;
    extern callback<const int8_t> font_change;
    extern callback<const int16_t> pixel_offset;
    extern callback<const mousebtn, const string> action_block_open;
    extern callback<const mousebtn> action_block_close;
    extern callback<const uint16_t> ascii_text_write;
    extern callback<const uint16_t> unicode_text_write;
    extern callback<const char*, const size_t> string_write;
  }
}

POLYBAR_NS_END
