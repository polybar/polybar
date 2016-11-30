#pragma once

#include <xpp/xpp.hpp>

#include "common.hpp"

POLYBAR_NS

class connection;
class registry;

using gcontext = xpp::gcontext<connection&>;
using pixmap = xpp::pixmap<connection&>;
using drawable = xpp::drawable<connection&>;
using colormap = xpp::colormap<connection&>;
using atom = xpp::atom<connection&>;
using font = xpp::font<connection&>;
using cursor = xpp::cursor<connection&>;

namespace reply_checked = xpp::x::reply::checked;
namespace reply_unchecked = xpp::x::reply::unchecked;
namespace reply {
  using get_atom_name = reply_checked::get_atom_name<connection&>;
}

POLYBAR_NS_END
