#pragma once

#include <mpris-generated.h>
#include <common.hpp>
#include <components/logger.hpp>

POLYBAR_NS

namespace mpris {

  class mprissong {
   public:
       mprissong(): mprissong("", "", "", -1) {}
       mprissong(string title, string album, string artist, unsigned duration)
           : title(title), album(album), artist(artist), duration(duration) {}

    const string title;
    const string album;
    const string artist;
    const unsigned duration;
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
    mprissong get_song();

   private:
    std::string player;
    std::string get(std::string property);
    PolybarOrgMprisMediaPlayer2Player* get_object();
    const logger& m_log;
  };
}

POLYBAR_NS_END
