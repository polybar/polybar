#pragma once

#include <components/logger.hpp>
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
    public:
        mprisconnection(const logger& m_log): m_log(m_log) {}
        mprissong get_current_song();
        void seek(int change);
        void set_random(bool mode);
    private:
        const logger& m_log;
};

}

POLYBAR_NS_END
