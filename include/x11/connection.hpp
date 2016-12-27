#pragma once

#include <xpp/core.hpp>
#include <xpp/generic/factory.hpp>
#include <xpp/proto/x.hpp>

#include "common.hpp"
#include "components/screen.hpp"
#include "x11/events.hpp"
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
    template <typename... Args>
    explicit connection_base(Args&&... args)
        : xpp::core::core(forward<Args>(args)...)
        , interfaces<connection_base<Derived, Extensions...>, Extensions...>(*this)
        , Extensions(static_cast<xcb_connection_t*>(*this))...
        , Extensions::error_dispatcher(static_cast<Extensions&>(*this).get())... {
      m_root_window = screen_of_display(default_screen())->root;
    }

    virtual ~connection_base() {}

    const Derived& operator=(const Derived& o) {
      return o;
    }

    virtual operator xcb_connection_t*() const {
      return *static_cast<const core&>(*this);
    }

    void operator()(const shared_ptr<xcb_generic_error_t>& error) const {
      check<xpp::x::extension, Extensions...>(error);
    }

    template <typename Extension>
    const Extension& extension() const {
      return static_cast<const Extension&>(*this);
    }

    template <typename WindowType = xcb_window_t>
    WindowType root() const {
      using make = xpp::generic::factory::make<Derived, xcb_window_t, WindowType>;
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
  static make_type make(xcb_connection_t* conn = nullptr);

  template <typename... Args>
  explicit connection(Args&&... args) : base_type::connection_base(forward<Args>(args)...) {}

  const connection& operator=(const connection& o) {
    return o;
  }

  // operator Display*() const;

  void preload_atoms();

  void query_extensions();

  string id(xcb_window_t w) const;

  xcb_screen_t* screen(bool realloc = false);

  void ensure_event_mask(xcb_window_t win, uint32_t event);

  void clear_event_mask(xcb_window_t win);

  shared_ptr<xcb_client_message_event_t> make_client_message(xcb_atom_t type, xcb_window_t target) const;

  void send_client_message(const shared_ptr<xcb_client_message_event_t>& message, xcb_window_t target,
      uint32_t event_mask = 0xFFFFFF, bool propagate = false) const;

  xcb_visualtype_t* visual_type(xcb_screen_t* screen, int match_depth = 32);

  static string error_str(int error_code);

  void dispatch_event(const shared_ptr<xcb_generic_event_t>& evt) const;

  template <typename Event, uint32_t ResponseType>
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
