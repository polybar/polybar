#pragma once

#include "common.hpp"
#include <dbus/dbus.h>

POLYBAR_NS

namespace mpris {

class mprissong {
  string get_artist();
  string get_album();
  string get_title();
  string get_date();
  unsigned get_duration();
};

class mprisconnection {
  mprissong get_current_song();
  void seek(int change);
  void set_random(bool mode);
};

}

POLYBAR_NS_END
