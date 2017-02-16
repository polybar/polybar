#pragma once

#include <mpris-generated.h>
#include <common.hpp>
#include <components/logger.hpp>

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
    mprisconnection(const logger& m_log, string player) : player(player), m_log(m_log) {}
    mprissong get_current_song();
    void pause_play();
    void seek(int change);
    void set_random(bool mode);
    std::string get_playback_status();
    std::map<std::string, std::string> get_metadata();

   private:
    std::string player;
    std::string get(std::string property);
    PolybarOrgMprisMediaPlayer2Player* get_object();
    const logger& m_log;
  };
}

POLYBAR_NS_END
