#pragma once

#include <mpd/client.h>
#include <stdlib.h>
#include <chrono>
#include <csignal>

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

// fwd
class logger;

namespace chrono = std::chrono;

namespace mpd {
  extern sig_atomic_t g_connection_closed;

  DEFINE_ERROR(mpd_exception);
  DEFINE_CHILD_ERROR(client_error, mpd_exception);
  DEFINE_CHILD_ERROR(server_error, mpd_exception);

  void check_connection(mpd_connection* conn);
  void check_errors(mpd_connection* conn);

  // types details {{{

  enum class connection_state { NONE = 0, CONNECTED, DISCONNECTED };

  enum class mpdstate {
    UNKNOWN = 1 << 0,
    STOPPED = 1 << 1,
    PLAYING = 1 << 2,
    PAUSED = 1 << 4,
  };

  namespace details {
    struct mpd_connection_deleter {
      void operator()(mpd_connection* conn);
    };

    struct mpd_status_deleter {
      void operator()(mpd_status* status);
    };

    struct mpd_song_deleter {
      void operator()(mpd_song* song);
    };
  }

  using mpd_connection_t = unique_ptr<mpd_connection, details::mpd_connection_deleter>;
  using mpd_status_t = unique_ptr<mpd_status, details::mpd_status_deleter>;
  using mpd_song_t = unique_ptr<mpd_song, details::mpd_song_deleter>;

  // }}}
  // class : mpdsong {{{

  class mpdsong {
   public:
    explicit mpdsong(mpd_song_t&& song) : m_song(forward<decltype(song)>(song)) {}

    operator bool();

    string get_artist();
    string get_album_artist();
    string get_album();
    string get_title();
    string get_date();
    unsigned get_duration();

   private:
    mpd_song_t m_song;
  };

  // }}}
  // class : mpdconnection {{{

  class mpdstatus;
  class mpdconnection {
   public:
    explicit mpdconnection(
        const logger& logger, string host, unsigned int port = 6600, string password = "", unsigned int timeout = 15);
    ~mpdconnection();

    void connect();
    void disconnect();
    bool connected();
    bool retry_connection(int interval = 1);

    int get_fd();
    void idle();
    int noidle();

    unique_ptr<mpdstatus> get_status();
    unique_ptr<mpdstatus> get_status_safe();
    unique_ptr<mpdsong> get_song();

    void play();
    void pause(bool state);
    void toggle();
    void stop();
    void prev();
    void next();
    void seek(int songid, int pos);

    void set_repeat(bool mode);
    void set_random(bool mode);
    void set_single(bool mode);
    void set_consume(bool mode);

    operator mpd_connection_t::element_type*();

   protected:
    void check_prerequisites();
    void check_prerequisites_commands_list();

   private:
    const logger& m_log;
    mpd_connection_t m_connection{};

    struct sigaction m_signal_action {};

    bool m_listactive = false;
    bool m_idle = false;
    int m_fd = -1;

    string m_host;
    unsigned int m_port;
    string m_password;
    unsigned int m_timeout;
  };

  // }}}
  // class : mpdstatus {{{

  class mpdstatus {
   public:
    explicit mpdstatus(mpdconnection* conn, bool autoupdate = true);

    void fetch_data(mpdconnection* conn);
    void update(int event, mpdconnection* connection);

    bool random() const;
    bool repeat() const;
    bool single() const;
    bool consume() const;

    bool match_state(mpdstate state) const;

    int get_songid() const;
    int get_queuelen() const;
    unsigned get_total_time() const;
    unsigned get_elapsed_time() const;
    unsigned get_elapsed_percentage();
    string get_formatted_elapsed();
    string get_formatted_total();
    int get_seek_position(int percentage);

   private:
    mpd_status_t m_status{};
    unique_ptr<mpdsong> m_song{};
    mpdstate m_state{mpdstate::UNKNOWN};

    bool m_random{false};
    bool m_repeat{false};
    bool m_single{false};
    bool m_consume{false};

    int m_songid{0};
    int m_queuelen{0};

    unsigned long m_total_time{0UL};
    unsigned long m_elapsed_time{0UL};
    unsigned long m_elapsed_time_ms{0UL};
  };

  // }}}
}

POLYBAR_NS_END
