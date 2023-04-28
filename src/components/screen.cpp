#include "components/screen.hpp"

#include <algorithm>
#include <csignal>
#include <thread>

#include "components/config.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"
#include "x11/registry.hpp"
#include "x11/types.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

using namespace signals::eventqueue;

/**
 * Create instance
 */
screen::make_type screen::make(const config& config) {
  return std::make_unique<screen>(connection::make(), signal_emitter::make(), logger::make(), config);
}

/**
 * Construct screen instance
 */
screen::screen(connection& conn, signal_emitter& emitter, const logger& logger, const config& conf)
    : m_connection(conn)
    , m_sig(emitter)
    , m_log(logger)
    , m_conf(conf)
    , m_root(conn.root())
    , m_monitors(randr_util::get_monitors(m_connection, true, false))
    , m_size({conn.screen()->width_in_pixels, conn.screen()->height_in_pixels}) {
  // Check if the reloading has been disabled by the user
  if (!m_conf.get("settings", "screenchange-reload", false)) {
    return;
  }

  // clang-format off
  m_proxy = winspec(m_connection)
    << cw_size(1U, 1U)
    << cw_pos(-1, -1)
    << cw_parent(m_root)
    << cw_params_override_redirect(true)
    << cw_params_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE)
    << cw_flush(true);
  // clang-format on

  // Update the root windows event mask
  auto attributes = m_connection.get_window_attributes(m_root);
  m_root_mask = attributes->your_event_mask;
  attributes->your_event_mask = attributes->your_event_mask | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
  m_connection.change_window_attributes(m_root, XCB_CW_EVENT_MASK, &attributes->your_event_mask);

  // Receive randr events
  m_connection.randr().select_input(m_proxy, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);

  // Attach the sink to process randr events
  m_connection.attach_sink(this, SINK_PRIORITY_SCREEN);

  // Create window used as event proxy
  m_connection.map_window(m_proxy);
  m_connection.flush();
}

/**
 * Deconstruct screen instance
 */
screen::~screen() {
  m_connection.detach_sink(this, SINK_PRIORITY_SCREEN);

  if (m_proxy != XCB_NONE) {
    m_connection.destroy_window(m_proxy);
  }
}

void screen::handle(const evt::map_notify& evt) {
  if (evt->window != m_proxy) {
    return;
  }

  // Once the proxy window has been mapped, restore the original root window event mask.
  m_connection.change_window_attributes(m_root, XCB_CW_EVENT_MASK, &m_root_mask);
}

/**
 * Handle XCB_RANDR_SCREEN_CHANGE_NOTIFY events
 *
 * If any of the monitors have changed we trigger a reload
 */
void screen::handle(const evt::randr_screen_change_notify& evt) {
  if (m_sigraised || evt->request_window != m_proxy) {
    return;
  }

  m_connection.reset_screen();
  auto screen = m_connection.screen();
  auto changed = false;

  // We need to reload if the screen size changed as well
  if (screen->width_in_pixels != m_size.w || screen->height_in_pixels != m_size.h) {
    changed = true;
  } else {
    changed = have_monitors_changed();
  }

  if (changed) {
    m_log.notice("randr_screen_change_notify (%ux%u)... reloading", evt->width, evt->height);
    m_sig.emit(exit_reload{});
    m_sigraised = true;
  }
}

/**
 * Checks if the stored monitor list is different from a newly fetched one
 *
 * Fetches the monitor list and compares it with the one stored
 */
bool screen::have_monitors_changed() const {
  auto monitors = randr_util::get_monitors(m_connection, true, false);

  if (monitors.size() != m_monitors.size()) {
    return true;
  }

  for (auto m : m_monitors) {
    auto it =
        std::find_if(monitors.begin(), monitors.end(), [m](auto& monitor) -> bool { return m->equals(*monitor); });

    /*
     * Every monitor in the stored list should also exist in the newly fetched
     * list. If this holds then the two lists are equivalent since they have
     * the same size
     */
    if (it == monitors.end()) {
      return true;
    }
  }

  return false;
}

POLYBAR_NS_END
