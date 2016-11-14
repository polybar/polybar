#include "components/signals.hpp"

LEMONBUDDY_NS

/**
 * Signals used to communicate with the bar window
 */
callback<string> g_signals::bar::action_click = nullptr;
callback<bool> g_signals::bar::visibility_change = nullptr;

/**
 * Signals used to communicate with the input parser
 */
callback<alignment> g_signals::parser::alignment_change = nullptr;
callback<attribute> g_signals::parser::attribute_set = nullptr;
callback<attribute> g_signals::parser::attribute_unset = nullptr;
callback<attribute> g_signals::parser::attribute_toggle = nullptr;
callback<mousebtn, string> g_signals::parser::action_block_open = nullptr;
callback<mousebtn> g_signals::parser::action_block_close = nullptr;
callback<gc, color> g_signals::parser::color_change = nullptr;
callback<int> g_signals::parser::font_change = nullptr;
callback<int> g_signals::parser::pixel_offset = nullptr;
callback<uint16_t> g_signals::parser::ascii_text_write = nullptr;
callback<uint16_t> g_signals::parser::unicode_text_write = nullptr;
callback<const char*, size_t> g_signals::parser::string_write = nullptr;

/**
 * Signals used to communicate with the tray manager
 */
callback<uint16_t> g_signals::tray::report_slotcount = nullptr;

LEMONBUDDY_NS_END
