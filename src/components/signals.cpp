#include "components/signals.hpp"
#include "components/types.hpp"

POLYBAR_NS

/**
 * Signals used to communicate with the bar window
 */
callback<const string> g_signals::bar::action_click{nullptr};
callback<const bool> g_signals::bar::visibility_change{nullptr};

/**
 * Signals used to communicate with the input parser
 */
callback<const uint32_t> g_signals::parser::background_change{nullptr};
callback<const uint32_t> g_signals::parser::foreground_change{nullptr};
callback<const uint32_t> g_signals::parser::underline_change{nullptr};
callback<const uint32_t> g_signals::parser::overline_change{nullptr};
callback<const alignment> g_signals::parser::alignment_change{nullptr};
callback<const attribute> g_signals::parser::attribute_set{nullptr};
callback<const attribute> g_signals::parser::attribute_unset{nullptr};
callback<const attribute> g_signals::parser::attribute_toggle{nullptr};
callback<const int8_t> g_signals::parser::font_change{nullptr};
callback<const int16_t> g_signals::parser::pixel_offset{nullptr};
callback<const mousebtn, const string> g_signals::parser::action_block_open{nullptr};
callback<const mousebtn> g_signals::parser::action_block_close{nullptr};
callback<const uint16_t> g_signals::parser::ascii_text_write{nullptr};
callback<const uint16_t> g_signals::parser::unicode_text_write{nullptr};
callback<const char*, const size_t> g_signals::parser::string_write{nullptr};

/**
 * Signals used to communicate with the tray manager
 */
callback<const uint16_t> g_signals::tray::report_slotcount{nullptr};

POLYBAR_NS_END
