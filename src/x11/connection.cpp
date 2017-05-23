#include <algorithm>
#include <iomanip>

#include "errors.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

/**
 * Create instance
 */
connection::make_type connection::make(xcb_connection_t* conn, int default_screen) {
  return static_cast<connection::make_type>(
      *factory_util::singleton<std::remove_reference_t<connection::make_type>>(conn, default_screen));
}

connection::connection(xcb_connection_t* c, int default_screen) : base_type(c, default_screen) {
  // Preload required xcb atoms {{{

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

// }}}
// Query for X extensions {{{
#if WITH_XDAMAGE
  damage_util::query_extension(*this);
#endif
#if WITH_XRENDER
  render_util::query_extension(*this);
#endif
#if WITH_XRANDR
  randr_util::query_extension(*this);
#endif
#if WITH_XSYNC
  sync_util::query_extension(*this);
#endif
#if WITH_XCOMPOSITE
  composite_util::query_extension(*this);
#endif
#if WITH_XKB
  xkb_util::query_extension(*this);
#endif
  // }}}
}

connection::~connection() {
  disconnect();
}

void connection::pack_values(unsigned int mask, const unsigned int* src, unsigned int* dest) {
  for (; mask; mask >>= 1, src++) {
    if (mask & 1) {
      *dest++ = *src;
    }
  }
}
void connection::pack_values(unsigned int mask, const xcb_params_cw_t* src, unsigned int* dest) {
  pack_values(mask, reinterpret_cast<const unsigned int*>(src), dest);
}
void connection::pack_values(unsigned int mask, const xcb_params_gc_t* src, unsigned int* dest) {
  pack_values(mask, reinterpret_cast<const unsigned int*>(src), dest);
}
void connection::pack_values(unsigned int mask, const xcb_params_configure_window_t* src, unsigned int* dest) {
  pack_values(mask, reinterpret_cast<const unsigned int*>(src), dest);
}

/**
 * Create X window id string
 */
string connection::id(xcb_window_t w) const {
  return sstream() << "0x" << std::hex << std::setw(7) << std::setfill('0') << w;
}

/**
 * Get pointer to the default xcb screen
 */
xcb_screen_t* connection::screen(bool realloc) {
  if (m_screen == nullptr || realloc) {
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
shared_ptr<xcb_client_message_event_t> connection::make_client_message(xcb_atom_t type, xcb_window_t target) const {
  auto client_message = memory_util::make_malloc_ptr<xcb_client_message_event_t, 32_z>();

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
void connection::send_client_message(const shared_ptr<xcb_client_message_event_t>& message, xcb_window_t target,
    unsigned int event_mask, bool propagate) const {
  send_event(propagate, target, event_mask, reinterpret_cast<const char*>(&*message));
  flush();
}

/**
 * Try to get a visual type for the given screen that
 * matches the given depth
 */
xcb_visualtype_t* connection::visual_type(xcb_screen_t* screen, int match_depth) {
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  if (depth_iter.data) {
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      if (match_depth == 0 || match_depth == depth_iter.data->depth) {
        for (auto it = xcb_depth_visuals_iterator(depth_iter.data); it.rem; xcb_visualtype_next(&it)) {
          return it.data;
        }
      }
    }
    if (match_depth > 0) {
      return visual_type(screen, 0);
    }
  }
  return nullptr;
}


xcb_visualtype_t* connection::visual_type_for_id(xcb_screen_t* screen, xcb_visualid_t visual_id) {
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  if (depth_iter.data) {
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      for (auto it = xcb_depth_visuals_iterator(depth_iter.data); it.rem; xcb_visualtype_next(&it)) {
        if(it.data->visual_id == visual_id) return it.data;
      }
    }
  }
  return nullptr;
}

/**
 * Query root window pixmap
 */
bool connection::root_pixmap(xcb_pixmap_t* pixmap, int* depth, xcb_rectangle_t* rect) {
  const xcb_atom_t pixmap_properties[3]{ESETROOT_PMAP_ID, _XROOTMAP_ID, _XSETROOT_ID};
  for (auto&& property : pixmap_properties) {
    try {
      auto prop = get_property(false, screen()->root, property, XCB_ATOM_PIXMAP, 0L, 1L);
      if (prop->format == 32 && prop->value_len == 1) {
        *pixmap = *prop.value<xcb_pixmap_t>().begin();
      }
    } catch (const exception& err) {
      continue;
    }
  }
  if (*pixmap) {
    try {
      auto geom = get_geometry(*pixmap);
      *depth = geom->depth;
      rect->width = geom->width;
      rect->height = geom->height;
      rect->x = geom->x;
      rect->y = geom->y;
      return true;
    } catch (const exception& err) {
      *pixmap = XCB_NONE;
      *rect = xcb_rectangle_t{0, 0, 0U, 0U};
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
