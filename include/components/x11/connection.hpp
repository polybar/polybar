#pragma once

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <iomanip>
#include <xpp/xpp.hpp>

#include "common.hpp"
#include "components/x11/atoms.hpp"
#include "components/x11/types.hpp"
#include "components/x11/xutils.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

using xpp_connection = xpp::connection<xpp::randr::extension>;

class connection : public xpp_connection {
 public:
  explicit connection() {}
  explicit connection(xcb_connection_t* conn) : xpp_connection(conn) {}

  connection& operator=(const connection&) {
    return *this;
  }

  virtual ~connection() {}

  /**
   * Preload required xcb atoms
   */
  auto preload_atoms() {
    for (auto&& a : ATOMS) *a.atom = intern_atom(false, a.len, a.name).atom();
  }

  /**
   * Check if required X extensions are available
   */
  auto query_extensions() {
    // damage().query_version(XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
    // if (!extension<xpp::damage::extension>()->present)
    //   throw application_error("Missing X extension: Damage");

    // render().query_version(XCB_RENDER_MAJOR_VERSION, XCB_RENDER_MINOR_VERSION);
    // if (!extension<xpp::render::extension>()->present)
    //   throw application_error("Missing X extension: Render");

    randr().query_version(XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
    if (!extension<xpp::randr::extension>()->present)
      throw application_error("Missing X extension: RandR");
  }

  /**
   * Create X window id string
   */
  auto id(xcb_window_t w) const {
    return string_util::from_stream(
        std::stringstream() << "0x" << std::hex << std::setw(7) << std::setfill('0') << w);
  }

  /**
   * Get pointer to the default xcb screen
   */
  auto screen() {
    if (m_screen == nullptr)
      m_screen = screen_of_display(default_screen());
    return m_screen;
  }

  /**
   * Creates an instance of shared_ptr<xcb_client_message_event_t>
   */
  auto make_client_message(xcb_atom_t type, xcb_window_t target) const {
    auto client_message = memory_util::make_malloc_ptr<xcb_client_message_event_t>(size_t{32});

    client_message->response_type = XCB_CLIENT_MESSAGE;
    client_message->format = 32;
    client_message->type = type;
    client_message->window = target;

    client_message->sequence = 0;
    client_message->data.data32[0] = 0;
    client_message->data.data32[1] = 0;
    client_message->data.data32[2] = 0;
    client_message->data.data32[3] = 0;
    client_message->data.data32[4] = 0;

    return client_message;
  }

  /**
   * Send client message event
   */
  void send_client_message(shared_ptr<xcb_client_message_event_t> message, xcb_window_t target,
      uint32_t event_mask = 0xFFFFFF, bool propagate = false) {
    send_event(propagate, target, event_mask, reinterpret_cast<char*>(message.get()));
    flush();
  }

  /**
   * Sends a dummy event to the specified window
   * Used to interrupt blocking wait call
   *
   * @XXX: Find the proper way to interrupt the blocking wait
   * except the obvious event polling
   */
  auto send_dummy_event(
      xcb_window_t target, uint32_t event = XCB_EVENT_MASK_STRUCTURE_NOTIFY) const {
    if (target == XCB_NONE)
      target = root();
    auto message = make_client_message(XCB_NONE, target);
    send_event(false, target, event, reinterpret_cast<char*>(message.get()));
    flush();
  }

  /**
   * Try to get a visual type for the given screen that
   * matches the given depth
   */
  optional<xcb_visualtype_t*> visual_type(xcb_screen_t* screen, int match_depth = 32) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    if (depth_iter.data) {
      for (; depth_iter.rem; xcb_depth_next(&depth_iter))
        if (match_depth == 0 || match_depth == depth_iter.data->depth)
          for (auto it = xcb_depth_visuals_iterator(depth_iter.data); it.rem;
               xcb_visualtype_next(&it))
            return it.data;
      if (match_depth > 0)
        return visual_type(screen, 0);
    }
    return {};
  }

  /**
   * Parse connection error
   */
  static string error_str(int error_code) {
    switch (error_code) {
      case XCB_CONN_ERROR:
        return "Socket, pipe or stream error";
      case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
        return "Unsupported extension";
      case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
        return "Not enough memory";
      case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
        return "Request length exceeded";
      case XCB_CONN_CLOSED_PARSE_ERR:
        return "Can't parse display string";
      case XCB_CONN_CLOSED_INVALID_SCREEN:
        return "Invalid screen";
      case XCB_CONN_CLOSED_FDPASSING_FAILED:
        return "Failed to pass FD";
      default:
        return "Unknown error";
    }
  }

  /**
   * Attach sink to the registry
   */
  template <typename Sink>
  void attach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.attach(prio, forward<Sink>(sink));
  }

  /**
   * Detach sink from the registry
   */
  template <typename Sink>
  void detach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.detach(prio, forward<Sink>(sink));
  }

  /**
   * Dispatch event through the registry
   */
  void dispatch_event(const shared_ptr<xcb_generic_event_t>& evt) {
    m_registry.dispatch(forward<decltype(evt)>(evt));
  }

  /**
   * Configure injection module
   */
  template <typename T = connection&>
  static di::injector<T> configure() {
    return di::make_injector(di::bind<>().to(
        factory::generic_singleton<lemonbuddy::connection>(xutils::get_connection())));
  }

 protected:
  registry m_registry{*this};
  xcb_screen_t* m_screen = nullptr;
};

LEMONBUDDY_NS_END
