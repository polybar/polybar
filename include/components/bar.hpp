#pragma once

#include <cstdlib>
#include <atomic>
#include <mutex>

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "utils/math.hpp"
#include "settings.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd {{{
class config;
class connection;
class logger;
class parser;
class renderer;
class screen;
class taskqueue;
class tray_manager;
// }}}

/**
 * Allows a new format for width, height, offset-x and offset-y in the bar section
 *
 * The new format is X%:Z, where X is in [0, 100], and Z is any real value
 * describing a pixel offset. The actual value is calculated by X% * max + Z
 * The max is the monitor width for width and offset-x and monitor height for
 * the other two
 */
inline double geom_format_to_pixels(std::string str, double max) {
  size_t i;
  if ((i = str.find(':')) != std::string::npos) {
    std::string a = str.substr(0, i - 1);
    std::string b = str.substr(i + 1);
    return math_util::percentage_to_value<double>(strtod(a.c_str(), nullptr), max) + strtod(b.c_str(), nullptr);
  } else {
    if (str.find('%') != std::string::npos) {
      return math_util::percentage_to_value<double>(strtod(str.c_str(), nullptr), max);
    } else {
      return strtod(str.c_str(), nullptr);
    }
  }
}

class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify, evt::enter_notify,
                evt::leave_notify, evt::motion_notify, evt::destroy_notify, evt::client_message, evt::configure_notify>,
            public signal_receiver<SIGN_PRIORITY_BAR, signals::eventqueue::start, signals::ui::tick,
                signals::ui::shade_window, signals::ui::unshade_window, signals::ui::dim_window
#if WITH_XCURSOR
                , signals::ui::cursor_change
#endif
		> {
 public:
  using make_type = unique_ptr<bar>;
  static make_type make(bool only_initialize_values = false);

  explicit bar(connection&, signal_emitter&, const config&, const logger&, unique_ptr<screen>&&,
      unique_ptr<tray_manager>&&, unique_ptr<parser>&&, unique_ptr<taskqueue>&&, bool only_initialize_values);
  ~bar();

  const bar_settings settings() const;

  void parse(string&& data, bool force = false);

  void hide();
  void show();
  void toggle();

 protected:
  void restack_window();
  void reconfigure_pos();
  void reconfigure_struts();
  void reconfigure_wm_hints();
  void broadcast_visibility();

  void handle(const evt::client_message& evt);
  void handle(const evt::destroy_notify& evt);
  void handle(const evt::enter_notify& evt);
  void handle(const evt::leave_notify& evt);
  void handle(const evt::motion_notify& evt);
  void handle(const evt::button_press& evt);
  void handle(const evt::expose& evt);
  void handle(const evt::property_notify& evt);
  void handle(const evt::configure_notify& evt);

  bool on(const signals::eventqueue::start&);
  bool on(const signals::ui::unshade_window&);
  bool on(const signals::ui::shade_window&);
  bool on(const signals::ui::tick&);
  bool on(const signals::ui::dim_window&);
#if WITH_XCURSOR
  bool on(const signals::ui::cursor_change&);
#endif

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;
  unique_ptr<parser> m_parser;
  unique_ptr<taskqueue> m_taskqueue;

  bar_settings m_opts{};

  string m_lastinput{};
  std::mutex m_mutex{};
  std::atomic<bool> m_dblclicks{false};

  mousebtn m_buttonpress_btn{mousebtn::NONE};
  int m_buttonpress_pos{0};
#if WITH_XCURSOR
  int m_motion_pos{0};
#endif

  event_timer m_buttonpress{0L, 5L};
  event_timer m_doubleclick{0L, 150L};

  double m_anim_step{0.0};

  bool m_visible{true};
};

POLYBAR_NS_END
