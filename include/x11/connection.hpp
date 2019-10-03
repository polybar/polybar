#pragma once

#include <xcb/xcb.h>
#include <cstdlib>
#include <xpp/core.hpp>
#include <xpp/generic/factory.hpp>
#include <xpp/proto/x.hpp>

#include "common.hpp"
#include "components/screen.hpp"
#include "x11/extensions/all.hpp"
#include "x11/registry.hpp"
#include "x11/types.hpp"

POLYBAR_NS

namespace detail {
  template <typename Connection, typename... Extensions>
  class interfaces : public xpp::x::extension::interface<interfaces<Connection, Extensions...>, Connection>,
                     public Extensions::template interface<interfaces<Connection, Extensions...>, Connection>... {
   public:
    const Connection& connection() const {
      return static_cast<const Connection&>(*this);
    }
  };

  template <typename Derived, typename... Extensions>
  class connection_base : public xpp::core,
                          public xpp::generic::error_dispatcher,
                          public detail::interfaces<connection_base<Derived, Extensions...>, Extensions...>,
                          private xpp::x::extension,
                          private xpp::x::extension::error_dispatcher,
                          private Extensions...,
                          private Extensions::error_dispatcher... {
   public:
    explicit connection_base(xcb_connection_t* c, int s)
        : xpp::core(c)
        , interfaces<connection_base<Derived, Extensions...>, Extensions...>(*this)
        , Extensions(m_c.get())...
        , Extensions::error_dispatcher(static_cast<Extensions&>(*this).get())... {
      core::m_screen = s;
      m_root_window = screen_of_display(default_screen())->root;
    }

    virtual ~connection_base() {}

    void operator()(const shared_ptr<xcb_generic_error_t>& error) const {
      check<xpp::x::extension, Extensions...>(error);
    }

    template <typename Extension>
    const Extension& extension() const {
      return static_cast<const Extension&>(*this);
    }

    template <typename WindowType = xcb_window_t>
    WindowType root() const {
      using make = xpp::generic::factory::make<connection_base, xcb_window_t, WindowType>;
      return make()(*this, m_root_window);
    }

    shared_ptr<xcb_generic_event_t> wait_for_event() const {
      try {
        return core::wait_for_event();
      } catch (const shared_ptr<xcb_generic_error_t>& error) {
        check<xpp::x::extension, Extensions...>(error);
      }
      throw;  // re-throw exception
    }

    shared_ptr<xcb_generic_event_t> wait_for_special_event(xcb_special_event_t* se) const {
      try {
        return core::wait_for_special_event(se);
      } catch (const shared_ptr<xcb_generic_error_t>& error) {
        check<xpp::x::extension, Extensions...>(error);
      }
      throw;  // re-throw exception
    }

   private:
    xcb_window_t m_root_window;

    template <typename Extension, typename Next, typename... Rest>
    void check(const shared_ptr<xcb_generic_error_t>& error) const {
      check<Extension>(error);
      check<Next, Rest...>(error);
    }

    template <typename Extension>
    void check(const shared_ptr<xcb_generic_error_t>& error) const {
      using error_dispatcher = typename Extension::error_dispatcher;
      auto& dispatcher = static_cast<const error_dispatcher&>(*this);
      dispatcher(error);
    }
  };
}

class connection : public detail::connection_base<connection&, XPP_EXTENSION_LIST> {
 public:
  using base_type = detail::connection_base<connection&, XPP_EXTENSION_LIST>;

  using make_type = connection&;
  static make_type make(xcb_connection_t* conn = nullptr, int default_screen = 0);

  explicit connection(xcb_connection_t* c, int default_screen);
  ~connection();

  const connection& operator=(const connection& o) {
    return o;
  }

  static void pack_values(unsigned int mask, const unsigned int* src, unsigned int* dest);
  static void pack_values(unsigned int mask, const xcb_params_cw_t* src, unsigned int* dest);
  static void pack_values(unsigned int mask, const xcb_params_gc_t* src, unsigned int* dest);
  static void pack_values(unsigned int mask, const xcb_params_configure_window_t* src, unsigned int* dest);

  xcb_screen_t* screen(bool realloc = false);

  string id(xcb_window_t w) const;

  void ensure_event_mask(xcb_window_t win, unsigned int event);
  void clear_event_mask(xcb_window_t win);

  shared_ptr<xcb_client_message_event_t> make_client_message(xcb_atom_t type, xcb_window_t target) const;
  void send_client_message(const shared_ptr<xcb_client_message_event_t>& message, xcb_window_t target,
      unsigned int event_mask = 0xFFFFFF, bool propagate = false) const;

  xcb_visualtype_t* visual_type(xcb_screen_t* screen, int match_depth = 32);
  xcb_visualtype_t* visual_type_for_id(xcb_screen_t* screen, xcb_visualid_t visual_id);

  bool root_pixmap(xcb_pixmap_t* pixmap, int* depth, xcb_rectangle_t* rect);

  static string error_str(int error_code);

  void dispatch_event(const shared_ptr<xcb_generic_event_t>& evt) const;

  template <typename Event, unsigned int ResponseType>
  void wait_for_response(function<bool(const Event*)> check_event) {
    int fd = get_file_descriptor();
    shared_ptr<xcb_generic_event_t> evt{};
    while (!connection_has_error()) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      if (!select(fd + 1, &fds, nullptr, nullptr, nullptr)) {
        continue;
      } else if ((evt = shared_ptr<xcb_generic_event_t>(xcb_poll_for_event(*this), free)) == nullptr) {
        continue;
      } else if (evt->response_type != ResponseType) {
        continue;
      } else if (check_event(reinterpret_cast<const Event*>(&*evt))) {
        break;
      }
    }
  }

  template <typename Sink>
  void attach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.attach(prio, forward<Sink>(sink));
  }

  template <typename Sink>
  void detach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.detach(prio, forward<Sink>(sink));
  }

 protected:
  registry m_registry{*this};
  xcb_screen_t* m_screen{nullptr};
};

POLYBAR_NS_END
