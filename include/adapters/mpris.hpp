#pragma once

#include <mpris-generated.h>
#include <common.hpp>
#include <components/logger.hpp>

POLYBAR_NS

namespace mpris {

  template <typename T>
  using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

  class mprissong {
   public:
    mprissong() : mprissong("", "", "") {}
    mprissong(string title, string album, string artist) : title(title), album(album), artist(artist) {}
    mprissong& operator=(mprissong other) {
      title = other.get_title();
      album = other.get_album();
      artist = other.get_artist();
      return *this;
    }

    bool operator==(mprissong other) {
      return title == other.get_title() && album == other.get_album() && artist == other.get_artist();
    }

    bool operator!=(mprissong other) {
      return !(*this == other);
    }

    // TODO: Macro ???
    string get_title() {
      return title;
    }
    string get_album() {
      return album;
    }
    string get_artist() {
      return artist;
    }

   private:
    string title;
    string album;
    string artist;
  };

  class mprisstatus {
   public:
    int position = -1;
    bool shuffle = false;
    string loop_status = "";
    string playback_status = "";
    string get_formatted_elapsed();
    string get_formatted_total();
    bool random() {
      return true;
    }
    bool repeat() {
      return true;
    }
    bool single() {
      return true;
    }
  };

  class mprisconnection {
   public:
    mprisconnection(const logger& m_log, string player) : player(player), m_log(m_log){};
    mprissong get_current_song();
    void pause_play();
    void seek(int change);
    void set_random(bool mode);
    std::map<std::string, std::string> get_metadata();
    void play();
    void pause();
    void stop();
    void prev();
    void next();
    bool connected();
    bool has_event();
    string get_loop_status();
    mprissong get_song();
    std::unique_ptr<mprisstatus> get_status();

   private:
    std::string player;
    std::string get(std::string property);
    string get_playback_status();
    deleted_unique_ptr<PolybarOrgMprisMediaPlayer2Player> get_object();
    const logger& m_log;
  };
}

POLYBAR_NS_END
