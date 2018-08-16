#pragma once

#include "common.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "events/types.hpp"
#include "x11/extensions/fwd.hpp"
#include "x11/types.hpp"

POLYBAR_NS

class logger;

namespace cairo {
  class surface;
  class xcb_surface;
}

/**
 * \brief Class to keep track of the desktop background used to support pseudo-transparency
 *
 * For pseudo-transparency that bar needs access to the desktop background.
 * We only need to store the slice of the background image which is covered by the bar window,
 * so this class takes a rectangle that limits what part of the background is stored.
 */
class background_manager : public signal_receiver<SIGN_PRIORITY_SCREEN, signals::ui::update_geometry>,
                           public xpp::event::sink<evt::property_notify>
{
 public:
  using make_type = background_manager&;
  static make_type make();

  /**
   * Initializes a new background_manager that by default does not observe anything.
   *
   * To observe a slice of the background you need to call background_manager::activate.
   */
  explicit background_manager(connection& conn, signal_emitter& sig, const logger& log);
  ~background_manager();

  /**
   * Starts observing a rectangular slice of the desktop background.
   *
   * After calling this function, you can obtain the current slice of the desktop background
   * with background_manager::get_surface. Whenever the background slice changes (for example,
   * due to bar position changes or because the user changed the desktop background) the class
   * emits a signals::ui::update_background event.
   *
   * \param window This should be set to the bar window.
   * \param rect Slice of the background to observe (coordinates relative to window).
   *             Typically set to the outer area of the bar.
   */
  void activate(xcb_window_t window, xcb_rectangle_t rect);


  /**
   * Stops observing the desktop background and frees all resources.
   */
  void deactivate();

  /**
   * Retrieve the current desktop background slice.
   *
   * This function returns a slice of the desktop background that has the size of the rectangle
   * given to background_manager::activate. As the slice is cached by the manager, this function
   * is fast.
   */
  cairo::surface* get_surface() const;

  void handle(const evt::property_notify& evt);
  bool on(const signals::ui::update_geometry&);
 private:
  // references to standard components
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;

  // these are set by activate
  xcb_window_t m_window;
  xcb_rectangle_t m_rect{0, 0, 0U, 0U};

  // required values for fetching the root window's background
  xcb_visualtype_t* m_visual{nullptr};
  xcb_gcontext_t m_gcontext{XCB_NONE};
  xcb_pixmap_t m_pixmap{XCB_NONE};
  unique_ptr<cairo::xcb_surface> m_surface;

  // true if we are currently attached as a listener for desktop background changes
  bool m_attached{false};

  void allocate_resources();
  void free_resources();
  void fetch_root_pixmap();

};

POLYBAR_NS_END
