#include "config.hpp"

#include "utils/factory.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

/**
 * Configure injection module
 */
di::injector<connection&> configure_connection() {
  return di::make_injector(di::bind<>().to(factory_util::generic_singleton<connection>(xutils::get_connection())));
}

/**
 * Preload required xcb atoms
 */
void connection::preload_atoms() {
  static bool s_atoms_loaded{false};

  if (s_atoms_loaded) {
    return;
  }

  vector<xcb_intern_atom_cookie_t> cookies(memory_util::countof(ATOMS));
  xcb_intern_atom_reply_t* reply{nullptr};

  for (size_t i = 0; i < cookies.size(); i++) {
    cookies[i] = xcb_intern_atom_unchecked(*this, false, ATOMS[i].len, ATOMS[i].name);
  }

  for (size_t i = 0; i < cookies.size(); i++) {
    if ((reply = xcb_intern_atom_reply(*this, cookies[i], nullptr)) != nullptr) {
      *ATOMS[i].atom = reply->atom;
    }

    free(reply);
  }

  s_atoms_loaded = true;
}

/**
 * Check if required X extensions are available
 */
void connection::query_extensions() {
  static bool s_extensions_loaded{false};

  if (s_extensions_loaded) {
    return;
  }

#ifdef ENABLE_DAMAGE_EXT
  damage().query_version(XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
  if (!extension<xpp::damage::extension>()->present)
    throw application_error("Missing X extension: Damage");
#endif
#ifdef ENABLE_RENDER_EXT
  render().query_version(XCB_RENDER_MAJOR_VERSION, XCB_RENDER_MINOR_VERSION);
  if (!extension<xpp::render::extension>()->present)
    throw application_error("Missing X extension: Render");
#endif
#ifdef ENABLE_RANDR_EXT
  randr().query_version(XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
  if (!extension<xpp::randr::extension>()->present)
    throw application_error("Missing X extension: RandR");
#endif

  s_extensions_loaded = true;
}

/**
 * Create X window id string
 */
string connection::id(xcb_window_t w) const {
  return string_util::from_stream(std::stringstream() << "0x" << std::hex << std::setw(7) << std::setfill('0') << w);
}

/**
 * Get pointer to the default xcb screen
 */
xcb_screen_t* connection::screen() {
  if (m_screen == nullptr)
    m_screen = screen_of_display(default_screen());
  return m_screen;
}

/**
 * Creates an instance of shared_ptr<xcb_client_message_event_t>
 */
shared_ptr<xcb_client_message_event_t> connection::make_client_message(xcb_atom_t type, xcb_window_t target) const {
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
void connection::send_client_message(
    shared_ptr<xcb_client_message_event_t> message, xcb_window_t target, uint32_t event_mask, bool propagate) const {
  const char* data = reinterpret_cast<decltype(data)>(message.get());
  send_event(propagate, target, event_mask, data);
  flush();
}

/**
 * Sends a dummy event to the specified window
 * Used to interrupt blocking wait call
 *
 * @XXX: Find the proper way to interrupt the blocking wait
 * except the obvious event polling
 */
void connection::send_dummy_event(xcb_window_t target, uint32_t event) const {
  if (target == XCB_NONE)
    target = root();
  auto message = make_client_message(XCB_VISIBILITY_NOTIFY, target);
  send_client_message(message, target, event);
}

/**
 * Try to get a visual type for the given screen that
 * matches the given depth
 */
boost::optional<xcb_visualtype_t*> connection::visual_type(xcb_screen_t* screen, int match_depth) {
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  if (depth_iter.data) {
    for (; depth_iter.rem; xcb_depth_next(&depth_iter))
      if (match_depth == 0 || match_depth == depth_iter.data->depth)
        for (auto it = xcb_depth_visuals_iterator(depth_iter.data); it.rem; xcb_visualtype_next(&it)) return it.data;
    if (match_depth > 0)
      return visual_type(screen, 0);
  }
  return {};
}

/**
 * Parse connection error
 */
string connection::error_str(int error_code) {
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
 * Dispatch event through the registry
 */
void connection::dispatch_event(const shared_ptr<xcb_generic_event_t>& evt) const {
  m_registry.dispatch(evt);
}

POLYBAR_NS_END
