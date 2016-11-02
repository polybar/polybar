#include "adapters/mpd.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace mpd {
  void check_connection(mpd_connection* conn) {
    if (conn == nullptr)
      throw client_error("Not connected to MPD server", MPD_ERROR_STATE);
  }

  void check_errors(mpd_connection* conn) {
    mpd_error code = mpd_connection_get_error(conn);

    if (code == MPD_ERROR_SUCCESS)
      return;

    auto msg = mpd_connection_get_error_message(conn);

    if (code == MPD_ERROR_SERVER) {
      mpd_connection_clear_error(conn);
      throw server_error(msg, mpd_connection_get_server_error(conn));
    } else {
      mpd_connection_clear_error(conn);
      throw client_error(msg, code);
    }
  }

  // deleters {{{

  namespace details {
    void mpd_connection_deleter::operator()(mpd_connection* conn) {
      if (conn != nullptr)
        mpd_connection_free(conn);
    }

    void mpd_status_deleter::operator()(mpd_status* status) {
      mpd_status_free(status);
    }

    void mpd_song_deleter::operator()(mpd_song* song) {
      mpd_song_free(song);
    }
  }

  // }}}
  // class: mpdsong {{{

  mpdsong::operator bool() {
    return m_song.get() != nullptr;
  }

  string mpdsong::get_artist() {
    assert(m_song);
    auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_ARTIST, 0);
    if (tag == nullptr)
      return "";
    return string{tag};
  }

  string mpdsong::get_album() {
    assert(m_song);
    auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_ALBUM, 0);
    if (tag == nullptr)
      return "";
    return string{tag};
  }

  string mpdsong::get_title() {
    assert(m_song);
    auto tag = mpd_song_get_tag(m_song.get(), MPD_TAG_TITLE, 0);
    if (tag == nullptr)
      return "";
    return string{tag};
  }

  unsigned mpdsong::get_duration() {
    assert(m_song);
    return mpd_song_get_duration(m_song.get());
  }

  // }}}
  // class: mpdconnection {{{

  void mpdconnection::connect() {
    try {
      m_log.trace("mpdconnection.connect: %s, %i, \"%s\", timeout: %i", m_host, m_port, m_password,
          m_timeout);
      m_connection.reset(mpd_connection_new(m_host.c_str(), m_port, m_timeout * 1000));
      check_errors(m_connection.get());

      if (!m_password.empty()) {
        noidle();
        assert(!m_listactive);
        mpd_run_password(m_connection.get(), m_password.c_str());
        check_errors(m_connection.get());
      }

      m_fd = mpd_connection_get_fd(m_connection.get());
      check_errors(m_connection.get());
    } catch (const client_error& e) {
      disconnect();
      throw e;
    }
  }

  void mpdconnection::disconnect() {
    m_connection.reset();
    m_idle = false;
    m_listactive = false;
  }

  bool mpdconnection::connected() {
    if (!m_connection)
      return false;
    return m_connection.get() != nullptr;
  }

  bool mpdconnection::retry_connection(int interval) {
    if (connected())
      return true;

    while (true) {
      try {
        connect();
        return true;
      } catch (const mpd_exception& e) {
      }

      this_thread::sleep_for(chrono::duration<double>(interval));
    }

    return false;
  }

  int mpdconnection::get_fd() {
    return m_fd;
  }

  void mpdconnection::idle() {
    check_connection(m_connection.get());
    if (m_idle)
      return;
    mpd_send_idle(m_connection.get());
    check_errors(m_connection.get());
    m_idle = true;
  }

  int mpdconnection::noidle() {
    check_connection(m_connection.get());
    int flags = 0;
    if (m_idle && mpd_send_noidle(m_connection.get())) {
      m_idle = false;
      flags = mpd_recv_idle(m_connection.get(), true);
      mpd_response_finish(m_connection.get());
      check_errors(m_connection.get());
    }
    return flags;
  }

  unique_ptr<mpdstatus> mpdconnection::get_status() {
    check_prerequisites();
    auto status = make_unique<mpdstatus>(this);
    check_errors(m_connection.get());
    // if (update)
    //   status->update(-1, this);
    return status;
  }

  unique_ptr<mpdstatus> mpdconnection::get_status_safe() {
    try {
      return get_status();
    } catch (const mpd_exception& e) {
      return {};
    }
  }

  unique_ptr<mpdsong> mpdconnection::get_song() {
    check_prerequisites_commands_list();
    mpd_send_current_song(m_connection.get());
    mpd_song_t song{mpd_recv_song(m_connection.get()), mpd_song_t::deleter_type{}};
    mpd_response_finish(m_connection.get());
    check_errors(m_connection.get());
    if (song.get() != nullptr) {
      return make_unique<mpdsong>(std::move(song));
    }
    return unique_ptr<mpdsong>{};
  }

  void mpdconnection::play() {
    try {
      check_prerequisites_commands_list();
      mpd_run_play(m_connection.get());
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.play: %s", e.what());
    }
  }

  void mpdconnection::pause(bool state) {
    try {
      check_prerequisites_commands_list();
      mpd_run_pause(m_connection.get(), state);
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.pause: %s", e.what());
    }
  }

  void mpdconnection::toggle() {
    try {
      check_prerequisites_commands_list();
      mpd_run_toggle_pause(m_connection.get());
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.toggle: %s", e.what());
    }
  }

  void mpdconnection::stop() {
    try {
      check_prerequisites_commands_list();
      mpd_run_stop(m_connection.get());
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.stop: %s", e.what());
    }
  }

  void mpdconnection::prev() {
    try {
      check_prerequisites_commands_list();
      mpd_run_previous(m_connection.get());
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.prev: %s", e.what());
    }
  }

  void mpdconnection::next() {
    try {
      check_prerequisites_commands_list();
      mpd_run_next(m_connection.get());
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.next: %s", e.what());
    }
  }

  void mpdconnection::seek(int songid, int pos) {
    try {
      check_prerequisites_commands_list();
      mpd_run_seek_id(m_connection.get(), songid, pos);
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.seek: %s", e.what());
    }
  }

  void mpdconnection::set_repeat(bool mode) {
    try {
      check_prerequisites_commands_list();
      mpd_run_repeat(m_connection.get(), mode);
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.set_repeat: %s", e.what());
    }
  }

  void mpdconnection::set_random(bool mode) {
    try {
      check_prerequisites_commands_list();
      mpd_run_random(m_connection.get(), mode);
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.set_random: %s", e.what());
    }
  }

  void mpdconnection::set_single(bool mode) {
    try {
      check_prerequisites_commands_list();
      mpd_run_single(m_connection.get(), mode);
      check_errors(m_connection.get());
    } catch (const mpd_exception& e) {
      m_log.err("mpdconnection.set_single: %s", e.what());
    }
  }

  mpdconnection::operator mpd_connection_t::element_type*() {
    return m_connection.get();
  }

  void mpdconnection::check_prerequisites() {
    check_connection(m_connection.get());
    noidle();
  }

  void mpdconnection::check_prerequisites_commands_list() {
    noidle();
    assert(!m_listactive);
    check_prerequisites();
  }

  // }}}
  // class: mpdstatus {{{

  mpdstatus::mpdstatus(mpdconnection* conn, bool autoupdate) {
    fetch_data(conn);
    if (autoupdate)
      update(-1, conn);
  }

  void mpdstatus::fetch_data(mpdconnection* conn) {
    m_status.reset(mpd_run_status(*conn));
    m_updated_at = chrono::system_clock::now();
    m_songid = mpd_status_get_song_id(m_status.get());
    m_random = mpd_status_get_random(m_status.get());
    m_repeat = mpd_status_get_repeat(m_status.get());
    m_single = mpd_status_get_single(m_status.get());
    m_elapsed_time = mpd_status_get_elapsed_time(m_status.get());
    m_total_time = mpd_status_get_total_time(m_status.get());
  }

  void mpdstatus::update(int event, mpdconnection* connection) {
    if (connection == nullptr || (event & (MPD_IDLE_PLAYER | MPD_IDLE_OPTIONS)) == false)
      return;

    fetch_data(connection);

    m_elapsed_time_ms = m_elapsed_time * 1000;

    auto state = mpd_status_get_state(m_status.get());

    switch (state) {
      case MPD_STATE_PAUSE:
        m_state = mpdstate::PAUSED;
        break;
      case MPD_STATE_PLAY:
        m_state = mpdstate::PLAYING;
        break;
      case MPD_STATE_STOP:
        m_state = mpdstate::STOPPED;
        break;
      default:
        m_state = mpdstate::UNKNOWN;
    }
  }

  void mpdstatus::update_timer() {
    auto diff = chrono::system_clock::now() - m_updated_at;
    auto dur = chrono::duration_cast<chrono::milliseconds>(diff);
    m_elapsed_time_ms += dur.count();
    m_elapsed_time = m_elapsed_time_ms / 1000 + 0.5f;
    m_updated_at = chrono::system_clock::now();
  }

  bool mpdstatus::random() const {
    return m_random;
  }

  bool mpdstatus::repeat() const {
    return m_repeat;
  }

  bool mpdstatus::single() const {
    return m_single;
  }

  bool mpdstatus::match_state(mpdstate state) const {
    return state == m_state;
  }

  int mpdstatus::get_songid() const {
    return m_songid;
  }

  unsigned mpdstatus::get_total_time() const {
    return m_total_time;
  }

  unsigned mpdstatus::get_elapsed_time() const {
    return m_elapsed_time;
  }

  unsigned mpdstatus::get_elapsed_percentage() {
    if (m_total_time == 0)
      return 0;
    return static_cast<int>(float(m_elapsed_time) / float(m_total_time) * 100.0 + 0.5f);
  }

  string mpdstatus::get_formatted_elapsed() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lu:%02lu", m_elapsed_time / 60, m_elapsed_time % 60);
    return {buffer};
  }

  string mpdstatus::get_formatted_total() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lu:%02lu", m_total_time / 60, m_total_time % 60);
    return {buffer};
  }

  int mpdstatus::get_seek_position(int percentage) {
    if (m_total_time == 0)
      return 0;
    math_util::cap<int>(0, 100, percentage);
    return float(m_total_time) * percentage / 100.0f + 0.5f;
  }

  // }}}
}

LEMONBUDDY_NS_END
