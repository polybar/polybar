#include <csignal>
#include <thread>

#include "components/config.hpp"
#include "components/logger.hpp"
#include "components/screen.hpp"
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
screen::make_type screen::make() {
  return factory_util::unique<screen>(connection::make(), signal_emitter::make(), logger::make(), config::make());
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
    , m_monitors(randr_util::get_monitors(m_connection, m_root, true))
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
  auto root_mask = attributes->your_event_mask;
  attributes->your_event_mask = attributes->your_event_mask | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
  m_connection.change_window_attributes(m_root, XCB_CW_EVENT_MASK, &attributes->your_event_mask);

  // Receive randr events
  m_connection.randr().select_input(m_proxy, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);

  // Create window used as event proxy
  m_connection.map_window(m_proxy);
  m_connection.flush();

  // Wait until the proxy window has been mapped
  using evt = xcb_map_notify_event_t;
  m_connection.wait_for_response<evt, XCB_MAP_NOTIFY>([&](const evt* evt) -> bool { return evt->window == m_proxy; });

  // Restore the root windows event mask
  m_connection.change_window_attributes(m_root, XCB_CW_EVENT_MASK, &root_mask);

  // Finally attach the sink the process randr events
  m_connection.attach_sink(this, SINK_PRIORITY_SCREEN);
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

/**
 * Handle XCB_RANDR_SCREEN_CHANGE_NOTIFY events
 *
 * If the screen dimensions have changed we raise USR1 to trigger a reload
 */
void screen::handle(const evt::randr_screen_change_notify& evt) {
  if (m_sigraised || evt->request_window != m_proxy) {
    return;
  }

  auto screen = m_connection.screen(true);
  auto changed = false;

  if (screen->width_in_pixels != m_size.w || screen->height_in_pixels != m_size.h) {
    changed = true;
  } else {
    auto monitors = randr_util::get_monitors(m_connection, m_root, true, true);
    for (size_t n = 0; n < monitors.size(); n++) {
      if (n < m_monitors.size() && monitors[n]->output != m_monitors[n]->output) {
        changed = true;
      }
    }
  }

  if (!changed) {
    return;
  }

  m_log.warn("randr_screen_change_notify (%ux%u)... reloading", evt->width, evt->height);
  m_sig.emit(exit_reload{});
  m_sigraised = true;
}

POLYBAR_NS_END
