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
    struct exit_reload;
    struct notify_change;
    struct notify_forcechange;
    struct check_state;
  } // namespace eventqueue
  namespace ipc {
    struct command;
    struct hook;
    struct action;
  } // namespace ipc
  namespace ui {
    struct changed;
    struct button_press;
    struct visibility_change;
    struct dim_window;
    struct request_snapshot;
    struct update_background;
    struct update_geometry;
  } // namespace ui
  namespace ui_tray {
    struct tray_pos_change;
  } // namespace ui_tray
} // namespace signals

POLYBAR_NS_END
