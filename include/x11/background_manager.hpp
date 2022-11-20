#pragma once

#include <memory>
#include <vector>

#include "common.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "x11/extensions/fwd.hpp"
#include "x11/types.hpp"

POLYBAR_NS

class logger;

namespace cairo {
  class surface;
  class xcb_surface;
} // namespace cairo

class bg_slice {
 public:
  ~bg_slice();
  // copying bg_slices is not allowed
  bg_slice(const bg_slice&) = delete;
  bg_slice& operator=(const bg_slice&) = delete;

  cairo::surface* get_surface() const;

  void clear();
  void copy(xcb_pixmap_t root_pixmap, int depth, xcb_rectangle_t geom, xcb_visualtype_t* visual);

 private:
  bg_slice(connection& conn, const logger& log, xcb_rectangle_t rect, xcb_window_t window);

  connection& m_connection;
  const logger& m_log;

  /**
   * Area covered by this slice
   *
   * Area is relative to given window
   */
  xcb_rectangle_t m_rect{0, 0, 0U, 0U};
  xcb_window_t m_window;

  /**
   * Cache for the root window background at this slice's position
   */
  xcb_pixmap_t m_pixmap{XCB_NONE};
  unique_ptr<cairo::xcb_surface> m_surface;
  xcb_gcontext_t m_gcontext{XCB_NONE};
  int m_depth{0};

  void ensure_resources(int depth, xcb_visualtype_t* visual);
  void allocate_resources(xcb_visualtype_t* visual);
  void free_resources();

  friend class background_manager;
};

/**
 * @brief Class to keep track of the desktop background used to support pseudo-transparency
 *
 * For pseudo-transparency that bar needs access to the desktop background.
 * We only need to store the slice of the background image which is covered by the bar window,
 * so this class takes a rectangle that limits what part of the background is stored.
 */
class background_manager : public signal_receiver<SIGN_PRIORITY_SCREEN, signals::ui::update_geometry>,
                           public xpp::event::sink<evt::property_notify> {
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
   * @param rect Slice of the background to observe (coordinates relative to window).
   * @param window Coordinates are interpreted relative to this window
   */
  std::shared_ptr<bg_slice> observe(xcb_rectangle_t rect, xcb_window_t window);

  void handle(const evt::property_notify& evt) override;
  bool on(const signals::ui::update_geometry&) override;

 private:
  void activate();
  void deactivate();

  void attach();
  void detach();
  /**
   * True if we are currently attached as a listener for desktop background changes
   */
  bool m_attached{false};

  // references to standard components
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;

  /**
   * List of slices that need to be filled with the desktop background
   */
  std::vector<std::weak_ptr<bg_slice>> m_slices;

  void allocate_resources();
  void free_resources();
  void on_background_change();

  void update_slice(bg_slice& slice);

  bool has_pixmap() const;
  void ensure_pixmap();
  void load_pixmap();
  void clear_pixmap();

  /**
   * The loaded root pixmap
   */
  xcb_pixmap_t m_pixmap{XCB_NONE};
  int m_pixmap_depth{0};
  xcb_rectangle_t m_pixmap_geom{0, 0, 0, 0};

  /**
   * Tracks whether we were able to load a pixmap.
   */
  bool m_pixmap_load_failed{false};

  /**
   * Visual matching the root pixmap's depth.
   *
   * Only valid if m_pixmap is set
   */
  xcb_visualtype_t* m_visual{nullptr};
};

POLYBAR_NS_END
