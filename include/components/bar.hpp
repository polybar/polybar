#pragma once

#include <atomic>
#include <cstdlib>
#include <mutex>

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "settings.hpp"
#include "tags/action_context.hpp"
#include "utils/math.hpp"
#include "utils/unit.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd {{{
class config;
class connection;
class logger;
class renderer;
class screen;
class taskqueue;
class tray_manager;

namespace tags {
  class dispatch;
}
// }}}

/**
 * Converts a percentage with offset into pixels
 */
inline unsigned int percentage_with_offset_to_pixel(percentage_with_offset g_format, double max, double dpi) {
  auto offset_pixel = unit_utils::extent_to_pixel(g_format.offset, dpi);

  return static_cast<unsigned int>(math_util::max<double>(
      0, math_util::percentage_to_value<double, double>(g_format.percentage, max) + offset_pixel));
}

// clang-format off
class bar : public xpp::event::sink<evt::button_press, evt::expose, evt::property_notify, evt::enter_notify,
              evt::leave_notify, evt::motion_notify, evt::destroy_notify, evt::client_message, evt::configure_notify>,
            public signal_receiver<SIGN_PRIORITY_BAR, signals::eventqueue::start, signals::ui::tick,
                signals::ui::shade_window, signals::ui::unshade_window, signals::ui::dim_window
#if WITH_XCURSOR
                ,
                signals::ui::cursor_change
#endif
                > {
 public:
  // clang-format on
  using make_type = unique_ptr<bar>;
  static make_type make(bool only_initialize_values = false);

  explicit bar(connection&, signal_emitter&, const config&, const logger&, unique_ptr<screen>&&,
      unique_ptr<tray_manager>&&, unique_ptr<tags::dispatch>&&, unique_ptr<tags::action_context>&&,
      unique_ptr<taskqueue>&&, bool only_initialize_values);
  ~bar();

  const bar_settings settings() const;

  void parse(string&& data, bool force = false);

  void hide();
  void show();
  void toggle();

 protected:
  void restack_window();
  void reconfigue_window();
  void reconfigure_geom();
  void reconfigure_pos();
  void reconfigure_struts();
  void reconfigure_wm_hints();
  void broadcast_visibility();

  void handle(const evt::client_message& evt) override;
  void handle(const evt::destroy_notify& evt) override;
  void handle(const evt::enter_notify& evt) override;
  void handle(const evt::leave_notify& evt) override;
  void handle(const evt::motion_notify& evt) override;
  void handle(const evt::button_press& evt) override;
  void handle(const evt::expose& evt) override;
  void handle(const evt::property_notify& evt) override;
  void handle(const evt::configure_notify& evt) override;

  bool on(const signals::eventqueue::start&) override;
  bool on(const signals::ui::unshade_window&) override;
  bool on(const signals::ui::shade_window&) override;
  bool on(const signals::ui::tick&) override;
  bool on(const signals::ui::dim_window&) override;
#if WITH_XCURSOR
  bool on(const signals::ui::cursor_change&) override;
#endif

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const config& m_conf;
  const logger& m_log;
  unique_ptr<screen> m_screen;
  unique_ptr<tray_manager> m_tray;
  unique_ptr<renderer> m_renderer;
  unique_ptr<tags::dispatch> m_dispatch;
  unique_ptr<tags::action_context> m_action_ctxt;
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
