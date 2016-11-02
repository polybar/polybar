#pragma once

#include "config.hpp"

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_util.h>
#ifdef ENABLE_DAMAGE_EXT
#include <xpp/proto/damage.hpp>
#endif
#ifdef ENABLE_RANDR_EXT
#include <xpp/proto/randr.hpp>
#endif
#ifdef ENABLE_RENDER_EXT
#include <xpp/proto/render.hpp>
#endif
#include <xpp/xpp.hpp>

#include "common.hpp"
#include "x11/connection.hpp"

LEMONBUDDY_NS

// xpp types {{{

class connection;

using registry = xpp::event::registry<connection&
#ifdef ENABLE_DAMAGE_EXT
    ,
    xpp::damage::extension
#endif
#ifdef ENABLE_RANDR_EXT
    ,
    xpp::randr::extension
#endif
#ifdef ENABLE_RENDER_EXT
    ,
    xpp::render::extension
#endif
    >;

using atom = xpp::atom<connection&>;
using colormap = xpp::colormap<connection&>;
using cursor = xpp::cursor<connection&>;
using drawable = xpp::drawable<connection&>;
using font = xpp::font<connection&>;
using gcontext = xpp::gcontext<connection&>;
using pixmap = xpp::pixmap<connection&>;

// }}}

namespace evt {
  // window focus events {{{

  using focus_in = xpp::x::event::focus_in<connection&>;
  using focus_out = xpp::x::event::focus_out<connection&>;

  // }}}
  // cursor events {{{

  using enter_notify = xpp::x::event::enter_notify<connection&>;
  using leave_notify = xpp::x::event::leave_notify<connection&>;
  using motion_notify = xpp::x::event::motion_notify<connection&>;

  // }}}
  // keyboard events {{{

  using button_press = xpp::x::event::button_press<connection&>;
  using button_release = xpp::x::event::button_release<connection&>;
  using key_press = xpp::x::event::key_press<connection&>;
  using key_release = xpp::x::event::key_release<connection&>;
  using keymap_notify = xpp::x::event::keymap_notify<connection&>;

  // }}}
  // render events {{{

  using circulate_notify = xpp::x::event::circulate_notify<connection&>;
  using circulate_request = xpp::x::event::circulate_request<connection&>;
  using colormap_notify = xpp::x::event::colormap_notify<connection&>;
  using configure_notify = xpp::x::event::configure_notify<connection&>;
  using configure_request = xpp::x::event::configure_request<connection&>;
  using create_notify = xpp::x::event::create_notify<connection&>;
  using destroy_notify = xpp::x::event::destroy_notify<connection&>;
  using expose = xpp::x::event::expose<connection&>;
  using graphics_exposure = xpp::x::event::graphics_exposure<connection&>;
  using gravity_notify = xpp::x::event::gravity_notify<connection&>;
  using map_notify = xpp::x::event::map_notify<connection&>;
  using map_request = xpp::x::event::map_request<connection&>;
  using mapping_notify = xpp::x::event::mapping_notify<connection&>;
  using no_exposure = xpp::x::event::no_exposure<connection&>;
  using reparent_notify = xpp::x::event::reparent_notify<connection&>;
  using resize_request = xpp::x::event::resize_request<connection&>;
  using unmap_notify = xpp::x::event::unmap_notify<connection&>;
  using visibility_notify = xpp::x::event::visibility_notify<connection&>;

  // }}}
  // data events {{{

  using client_message = xpp::x::event::client_message<connection&>;
  using ge_generic = xpp::x::event::ge_generic<connection&>;
  using property_notify = xpp::x::event::property_notify<connection&>;

  // }}}
  // selection events {{{

  using selection_clear = xpp::x::event::selection_clear<connection&>;
  using selection_notify = xpp::x::event::selection_notify<connection&>;
  using selection_request = xpp::x::event::selection_request<connection&>;

// }}}

#ifdef ENABLE_RANDR_EXT
  using randr_notify = xpp::randr::event::notify<connection&>;
  using randr_screen_change_notify = xpp::randr::event::screen_change_notify<connection&>;
#endif
}

LEMONBUDDY_NS_END
