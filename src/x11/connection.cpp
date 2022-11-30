#include "x11/connection.hpp"

#include <algorithm>
#include <iomanip>

#include "errors.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"

POLYBAR_NS

/**
 * Create instance
 */
connection::make_type connection::make(xcb_connection_t* conn, int default_screen) {
  return static_cast<connection::make_type>(
      *factory_util::singleton<std::remove_reference_t<connection::make_type>>(conn, default_screen));
}

connection::connection(xcb_connection_t* c, int default_screen) : base_type(c, default_screen) {
  // Preload required xcb atoms
  vector<xcb_intern_atom_cookie_t> cookies(ATOMS.size());

  for (size_t i = 0; i < cookies.size(); i++) {
    cookies[i] = xcb_intern_atom_unchecked(*this, false, ATOMS[i].name.size(), ATOMS[i].name.data());
  }

  for (size_t i = 0; i < cookies.size(); i++) {
    malloc_unique_ptr<xcb_intern_atom_reply_t> reply(xcb_intern_atom_reply(*this, cookies[i], nullptr), free);
    if (reply) {
      ATOMS[i].atom = reply->atom;
    }
  }

// Query for X extensions
#if WITH_XRANDR
  randr_util::query_extension(*this);
#endif
#if WITH_XCOMPOSITE
  composite_util::query_extension(*this);
#endif
#if WITH_XKB
  xkb_util::query_extension(*this);
#endif
}

connection::~connection() {
  disconnect();
}

/**
 * Packs data in src into the dest array.
 *
 * Each value in src is transferred into dest, if the corresponding bit in the
 * mask is set.
 *
 * Required if parameters were set using XCB_AUX_ADD_PARAM but a value_list is needed in the function call.
 *
 * @param mask bitmask specifying which entries in src are selected
 * @param src Array of 32-bit integers. Must have at least as many entries as the highest bit set in mask
 * @param dest Entries from src are packed into this array
 */
void connection::pack_values(uint32_t mask, const void* src, std::array<uint32_t, 32>& dest) {
  size_t dest_i = 0;
  for (size_t i = 0; i < dest.size() && mask; i++, mask >>= 1) {
    if (mask & 0x1) {
      dest[dest_i] = reinterpret_cast<const uint32_t*>(src)[i];
      dest_i++;
    }
  }
}

/**
 * Create X window id string
 */
string connection::id(xcb_window_t w) const {
  return sstream() << "0x" << std::hex << std::setw(7) << std::setfill('0') << w;
}

void connection::reset_screen() {
  m_screen = nullptr;
}

/**
 * Get pointer to the default xcb screen
 */
xcb_screen_t* connection::screen() {
  if (m_screen == nullptr) {
    m_screen = screen_of_display(default_screen());
  }
  return m_screen;
}

/**
 * Add given event to the event mask unless already added
 */
void connection::ensure_event_mask(xcb_window_t win, unsigned int event) {
  auto attributes = get_window_attributes(win);
  attributes->your_event_mask = attributes->your_event_mask | event;
  change_window_attributes(win, XCB_CW_EVENT_MASK, &attributes->your_event_mask);
}

/**
 * Clear event mask for the given window
 */
void connection::clear_event_mask(xcb_window_t win) {
  unsigned int mask{XCB_EVENT_MASK_NO_EVENT};
  change_window_attributes(win, XCB_CW_EVENT_MASK, &mask);
}

/**
 * Creates an instance of shared_ptr<xcb_client_message_event_t>
 */
xcb_client_message_event_t connection::make_client_message(xcb_atom_t type, xcb_window_t target) const {
  xcb_client_message_event_t client_message;
  client_message.response_type = XCB_CLIENT_MESSAGE;
  client_message.format = 32;
  client_message.type = type;
  client_message.window = target;

  client_message.sequence = 0;
  client_message.data.data32[0] = 0;
  client_message.data.data32[1] = 0;
  client_message.data.data32[2] = 0;
  client_message.data.data32[3] = 0;
  client_message.data.data32[4] = 0;

  return client_message;
}

/**
 * Send client message event
 */
void connection::send_client_message(
    const xcb_client_message_event_t& message, xcb_window_t target, unsigned int event_mask, bool propagate) const {
  send_event(propagate, target, event_mask, reinterpret_cast<const char*>(&message));
  flush();
}

/**
 * Try to get a visual type for the given screen that
 * matches the given depth
 */
xcb_visualtype_t* connection::visual_type(xcb_visual_class_t class_, int match_depth) {
  return xcb_aux_find_visual_by_attrs(screen(), class_, match_depth);
}

xcb_visualtype_t* connection::visual_type_for_id(xcb_visualid_t visual_id) {
  return xcb_aux_find_visual_by_id(screen(), visual_id);
}

/**
 * Query root window pixmap
 */
bool connection::root_pixmap(xcb_pixmap_t* pixmap, int* depth, xcb_rectangle_t* rect) {
  *pixmap = XCB_NONE; // default value if getting the root pixmap fails

  /*
   * We try multiple properties for the root pixmap here because I am not 100% sure
   * if all programs set them the same way. We might be able to just use _XSETROOT_ID
   * but keeping the other as fallback should not hurt (if it does, feel free to remove).
   *
   * see https://metacpan.org/pod/X11::Protocol::XSetRoot#ROOT-WINDOW-PROPERTIES for description of the properties
   * the properties here are ordered by reliability:
   *    _XSETROOT_ID: this is usually a dummy 1x1 pixmap only for resource managment, use only as last resort
   *    ESETROOT_PMAP_ID: according to the link above, this should usually by equal to _XROOTPMAP_ID
   *    _XROOTPMAP_ID: this appears to be the "correct" property to use? if available, use this
   */
  const xcb_atom_t pixmap_properties[3]{_XROOTPMAP_ID, ESETROOT_PMAP_ID, _XSETROOT_ID};
  for (auto&& property : pixmap_properties) {
    try {
      auto prop = get_property(false, root(), property, XCB_ATOM_PIXMAP, 0L, 1L);
      if (prop->format == 32 && prop->value_len == 1) {
        *pixmap = *prop.value<xcb_pixmap_t>().begin();
      }

      if (*pixmap) {
        auto geom = get_geometry(*pixmap);
        *depth = geom->depth;
        rect->width = geom->width;
        rect->height = geom->height;
        rect->x = geom->x;
        rect->y = geom->y;
        return true;
      }
    } catch (const exception& err) {
      *pixmap = XCB_NONE;
      *rect = xcb_rectangle_t{0, 0, 0U, 0U};
      continue;
    }
  }
  return false;
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
