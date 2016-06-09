#pragma once

#include <string>
#include <stdlib.h>
#include <memory>
#include <chrono>
#include <mpd/client.h>

#include "config.hpp"
#include "lemonbuddy.hpp"
#include "exception.hpp"
#include "utils/math.hpp"

namespace mpd
{
  class Exception : public ::Exception
  {
    public:
      Exception(const std::string& msg, bool clearable)
        : ::Exception(msg + (clearable ? " (clearable)" : " (not clearable)")){}
  };

  class ClientError : public Exception
  {
    public:
      explicit ClientError(const std::string& msg, mpd_error code, bool clearable)
        : Exception("[mpd::ClientError::"+ std::to_string(code) +"] "+ msg, clearable){}
  };

  class ServerError : public Exception
  {
    public:
      ServerError(const std::string& msg, mpd_server_error code, bool clearable)
        : Exception("[mpd::ServerError::"+ std::to_string(code) +"] "+ msg, clearable){}
  };

  enum State
  {
    UNKNOWN = 1 << 0,
    STOPPED = 1 << 1,
    PLAYING = 1 << 2,
    PAUSED  = 1 << 4,
  };

  struct Song
  {
    Song(){}
    explicit Song(mpd_song *song);

    std::shared_ptr<mpd_song> song;

    std::string get_artist();
    std::string get_album();
    std::string get_title();
    // unsigned get_duration();

    operator bool() {
      return this->song.get() != nullptr;
    }
  };

  class Connection;

  struct Status
  {
    struct StatusDeleter
    {
      void operator()(mpd_status *status) {
        mpd_status_free(status);
      }
    };

    explicit Status(mpd_status *status);

    std::unique_ptr<struct mpd_status, StatusDeleter> status;
    std::unique_ptr<Song> song;

    std::chrono::system_clock::time_point updated_at;

    int state = UNKNOWN;

    bool random = false,
         repeat = false,
         single = false;

    int song_id;

    unsigned long total_time;
    unsigned long elapsed_time;
    unsigned long elapsed_time_ms;

    void set(std::unique_ptr<struct mpd_status, StatusDeleter> status);

    void update(int event, std::unique_ptr<Connection>& connection);
    void update_timer();

    // unsigned get_total_time();
    // unsigned get_elapsed_time();
    unsigned get_elapsed_percentage();
    std::string get_formatted_elapsed();
    std::string get_formatted_total();
  };

  class Connection
  {
    struct ConnectionDeleter
    {
      void operator()(mpd_connection *connection)
      {
        if (connection == nullptr)
          return;
        //TRACE("Releasing mpd_connection");
        mpd_connection_free(connection);
      }
    };

    std::unique_ptr<mpd_connection, ConnectionDeleter> connection;
    std::string host;
    std::string password;
    int port;
    int timeout = 15;

    bool mpd_command_list_active = false;
    bool mpd_idle = false;
    int mpd_fd;

    void check_connection();
    void check_prerequisites();
    void check_prerequisites_commands_list();
    void check_errors();

    public:
      Connection(std::string host, int port, std::string password)
        : host(host), password(password), port(port) {}

      void connect();
      void disconnect();
      bool connected();
      // bool retry_connection(int interval = 1);
      void idle();
      int noidle();

      void set_host(const std::string& host) { this->host = host; }
      void set_port(int port) { this->port = port; }
      void set_password(const std::string& password) { this->password = password; }
      void set_timeout(int timeout) { this->timeout = timeout; }

      std::unique_ptr<Status> get_status();
      std::unique_ptr<Song> get_song();

      void play();
      void pause(bool state);
      // void toggle();
      void stop();
      void prev();
      void next();
      void seek(int percentage);

      void set_repeat(bool mode);
      void set_random(bool mode);
      void set_single(bool mode);
  };

  struct MpdStatus
  {
    bool random, repeat, single;

    std::string artist;
    std::string album;
    std::string title;

    int elapsed_time = 0;
    int total_time = 0;

    float get_elapsed_percentage();

    std::string get_formatted_elapsed();
    std::string get_formatted_total();

    operator bool() {
      return !this->artist.empty() && !this->title.empty();
    }
  };
}
