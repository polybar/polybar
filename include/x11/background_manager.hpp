#pragma once

#include <memory>
#include <vector>

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

class bg_slice {
 public:
  ~bg_slice();
  // copying bg_slices is not allowed
  bg_slice(const bg_slice&) = delete;
  bg_slice& operator=(const bg_slice&) = delete;

  /**
   * Get the current desktop background at the location of this slice.
   * The returned pointer is only valid as long as the slice itself is alive.
   *
   * This function is fast, since the current desktop background is cached.
   */
  cairo::surface* get_surface() const {
    return m_surface.get();
  }

 private:
  bg_slice(connection& conn, const logger& log, xcb_rectangle_t rect, xcb_window_t window, xcb_visualtype_t* visual);

  // standard components
  connection& m_connection;

  // area covered by this slice
  xcb_rectangle_t m_rect{0, 0, 0U, 0U};
  xcb_window_t m_window;

  // cache for the root window background at this slice's position
  xcb_pixmap_t m_pixmap{XCB_NONE};
  unique_ptr<cairo::xcb_surface> m_surface;
  xcb_gcontext_t m_gcontext{XCB_NONE};

  void allocate_resources(const logger& log, xcb_visualtype_t* visual);
  void free_resources();

  friend class background_manager;
};

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
   * by calling get_surface on the returned bg_slice object.
   * Whenever the background slice changes (for example, due to bar position changes or because
   * the user changed the desktop background) the class emits a signals::ui::update_background event.
   *
   * You should only call this function once and then re-use the returned bg_slice because the bg_slice
   * caches the background. If you don't need the background anymore, destroy the shared_ptr to free up
   * resources.
   *
   * \param rect Slice of the background to observe (coordinates relative to window).
   * \param window Coordinates are interpreted relative to this window
   */
  std::shared_ptr<bg_slice> observe(xcb_rectangle_t rect, xcb_window_t window);

  void handle(const evt::property_notify& evt);
  bool on(const signals::ui::update_geometry&);
 private:
  void activate();
  void deactivate();

  // references to standard components
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;

  // list of slices that need to be filled with the desktop background
  std::vector<std::weak_ptr<bg_slice>> m_slices;

  // required values for fetching the root window's background
  xcb_visualtype_t* m_visual{nullptr};

  // true if we are currently attached as a listener for desktop background changes
  bool m_attached{false};

  void allocate_resources();
  void free_resources();
  void fetch_root_pixmap();

};

POLYBAR_NS_END
