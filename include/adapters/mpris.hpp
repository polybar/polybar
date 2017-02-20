#pragma once

#include <mpris-generated.h>
#include <common.hpp>
#include <components/logger.hpp>

POLYBAR_NS

namespace mpris {

  class mprissong {
   public:
    mprissong() : mprissong("", "", "") {}
    mprissong(string title, string album, string artist) : title(title), album(album), artist(artist) {}

    //TODO: Macro ???
    auto get_title() {
      return title;
    }
    auto get_album() {
      return album;
    }
    auto get_artist() {
      return artist;
    }

   private:
    const string title;
    const string album;
    const string artist;
  };

  class mprisstatus {
   public:
    int position = -1;
    bool shuffle = false;
    string loop_status = "";
    string playback_status = "";
    string get_formatted_elapsed();
    string get_formatted_total();
    bool random();
    bool repeat();
    bool single();
  };

  class mprisconnection {
   public:
    mprisconnection(const logger& m_log, string player) : player(player), m_log(m_log) {}
    mprissong get_current_song();
    void pause_play();
    void seek(int change);
    void set_random(bool mode);
    std::map<std::string, std::string> get_metadata();
    void stop();
    bool connected();
    bool has_event();
    string get_loop_status();
    mprissong get_song();
    mprisstatus get_status();
    const mprisstatus* status = new mprisstatus();

   private:
    std::string player;
    std::string get(std::string property);
    string get_playback_status();
    PolybarOrgMprisMediaPlayer2Player* get_object();
    const logger& m_log;
  };
}

POLYBAR_NS_END
