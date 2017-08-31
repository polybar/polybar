#pragma once

#include "common.hpp"

POLYBAR_NS

class signal_emitter;
class signal_receiver_interface;
template <int Priority, typename Signal, typename... Signals>
class signal_receiver;

namespace signals {
  namespace detail {
    class signal;
  }

  namespace eventqueue {
    struct start;
    struct exit_terminate;
    struct exit_reload;
    struct notify_change;
    struct notify_forcechange;
    struct check_state;
  }
  namespace ipc {
    struct command;
    struct hook;
    struct action;
  }
  namespace ui {
    struct ready;
    struct changed;
    struct tick;
    struct button_press;
    struct cursor_change;
    struct visibility_change;
    struct dim_window;
    struct shade_window;
    struct unshade_window;
    struct request_snapshot;
    struct update_background;
    struct update_geometry;
  }
  namespace ui_tray {
    struct mapped_clients;
  }
  namespace parser {
    struct change_background;
    struct change_foreground;
    struct change_underline;
    struct change_overline;
    struct change_font;
    struct change_alignment;
    struct reverse_colors;
    struct offset_pixel;
    struct attribute_set;
    struct attribute_unset;
    struct attribute_toggle;
    struct action_begin;
    struct action_end;
    struct text;
  }
}

POLYBAR_NS_END
