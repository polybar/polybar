#include "modules/mpd.hpp"

LEMONBUDDY_NS

using namespace mpd;

namespace modules {
  void mpd_module::setup() {
    m_host = m_conf.get<string>(name(), "host", m_host);
    m_port = m_conf.get<unsigned int>(name(), "port", m_port);
    m_pass = m_conf.get<string>(name(), "password", m_pass);
    m_synctime = m_conf.get<float>(name(), "interval", m_synctime);

    // Add formats and elements {{{

    m_formatter->add(FORMAT_ONLINE, TAG_LABEL_SONG,
        {TAG_BAR_PROGRESS, TAG_TOGGLE, TAG_LABEL_SONG, TAG_LABEL_TIME, TAG_ICON_RANDOM,
            TAG_ICON_REPEAT, TAG_ICON_REPEAT_ONE, TAG_ICON_PREV, TAG_ICON_STOP, TAG_ICON_PLAY,
            TAG_ICON_PAUSE, TAG_ICON_NEXT, TAG_ICON_SEEKB, TAG_ICON_SEEKF});

    m_formatter->add(FORMAT_OFFLINE, "", {TAG_LABEL_OFFLINE});

    m_icons = iconset_t{new iconset()};

    if (m_formatter->has(TAG_ICON_PLAY) || m_formatter->has(TAG_TOGGLE))
      m_icons->add("play", load_icon(m_conf, name(), TAG_ICON_PLAY));
    if (m_formatter->has(TAG_ICON_PAUSE) || m_formatter->has(TAG_TOGGLE))
      m_icons->add("pause", load_icon(m_conf, name(), TAG_ICON_PAUSE));
    if (m_formatter->has(TAG_ICON_STOP))
      m_icons->add("stop", load_icon(m_conf, name(), TAG_ICON_STOP));
    if (m_formatter->has(TAG_ICON_PREV))
      m_icons->add("prev", load_icon(m_conf, name(), TAG_ICON_PREV));
    if (m_formatter->has(TAG_ICON_NEXT))
      m_icons->add("next", load_icon(m_conf, name(), TAG_ICON_NEXT));
    if (m_formatter->has(TAG_ICON_SEEKB))
      m_icons->add("seekb", load_icon(m_conf, name(), TAG_ICON_SEEKB));
    if (m_formatter->has(TAG_ICON_SEEKF))
      m_icons->add("seekf", load_icon(m_conf, name(), TAG_ICON_SEEKF));
    if (m_formatter->has(TAG_ICON_RANDOM))
      m_icons->add("random", load_icon(m_conf, name(), TAG_ICON_RANDOM));
    if (m_formatter->has(TAG_ICON_REPEAT))
      m_icons->add("repeat", load_icon(m_conf, name(), TAG_ICON_REPEAT));
    if (m_formatter->has(TAG_ICON_REPEAT_ONE))
      m_icons->add("repeat_one", load_icon(m_conf, name(), TAG_ICON_REPEAT_ONE));

    if (m_formatter->has(TAG_LABEL_SONG))
      m_label_song = load_optional_label(m_conf, name(), TAG_LABEL_SONG, "%artist% - %title%");
    if (m_formatter->has(TAG_LABEL_TIME))
      m_label_time = load_optional_label(m_conf, name(), TAG_LABEL_TIME, "%elapsed% / %total%");
    if (m_formatter->has(TAG_ICON_RANDOM) || m_formatter->has(TAG_ICON_REPEAT) ||
        m_formatter->has(TAG_ICON_REPEAT_ONE)) {
      m_toggle_on_color = m_conf.get<string>(name(), "toggle-on-foreground", "");
      m_toggle_off_color = m_conf.get<string>(name(), "toggle-off-foreground", "");
    }
    if (m_formatter->has(TAG_LABEL_OFFLINE, FORMAT_OFFLINE))
      m_label_offline = load_label(m_conf, name(), TAG_LABEL_OFFLINE);
    if (m_formatter->has(TAG_BAR_PROGRESS))
      m_bar_progress = load_progressbar(m_bar, m_conf, name(), TAG_BAR_PROGRESS);

    // }}}

    m_lastsync = chrono::system_clock::now();

    try {
      m_mpd = make_unique<mpdconnection>(m_log, m_host, m_port, m_pass);
      m_mpd->connect();
      m_status = m_mpd->get_status();
    } catch (const mpd_exception& err) {
      m_log.err("%s: %s", name(), err.what());
      m_mpd.reset();
    }
  }

  void mpd_module::teardown() {
    m_mpd.reset();
  }

  inline bool mpd_module::connected() const {
    return m_mpd && m_mpd->connected();
  }

  void mpd_module::idle() {
    if (connected())
      sleep(80ms);
    else {
      sleep(2s);
    }
  }

  bool mpd_module::has_event() {
    bool def = false;

    if (!connected() && m_statebroadcasted == mpd::connection_state::CONNECTED) {
      def = true;
    } else if (connected() && m_statebroadcasted == mpd::connection_state::DISCONNECTED) {
      def = true;
    }

    try {
      if (!m_mpd)
        m_mpd = make_unique<mpdconnection>(m_log, m_host, m_port, m_pass);
      if (!connected())
        m_mpd->connect();
    } catch (const mpd_exception& err) {
      m_log.trace("%s: %s", name(), err.what());
      m_mpd.reset();
      return def;
    }

    if (!connected())
      return def;

    if (!m_status)
      m_status = m_mpd->get_status_safe();

    try {
      m_mpd->idle();

      int idle_flags = 0;

      if ((idle_flags = m_mpd->noidle()) != 0) {
        m_status->update(idle_flags, m_mpd.get());
        return true;
      }

      if (m_status->match_state(mpdstate::PLAYING)) {
        m_status->update_timer();
      }
    } catch (const mpd_exception& err) {
      m_log.err(err.what());
      m_mpd.reset();
      return def;
    }

    if ((m_label_time || m_bar_progress) && m_status->match_state(mpdstate::PLAYING)) {
      auto now = chrono::system_clock::now();
      auto diff = now - m_lastsync;

      if (chrono::duration_cast<chrono::milliseconds>(diff).count() > m_synctime * 1000) {
        m_lastsync = now;
        return true;
      }
    }

    return def;
  }

  bool mpd_module::update() {
    if (connected())
      m_statebroadcasted = mpd::connection_state::CONNECTED;
    else if (!connected() && m_statebroadcasted != mpd::connection_state::DISCONNECTED)
      m_statebroadcasted = mpd::connection_state::DISCONNECTED;
    else if (!connected())
      return false;

    if (!m_status) {
      if (connected() && (m_status = m_mpd->get_status_safe())) {
        return false;
      }
    }

    string artist;
    string album;
    string title;
    string elapsed_str;
    string total_str;

    try {
      if (m_status) {
        elapsed_str = m_status->get_formatted_elapsed();
        total_str = m_status->get_formatted_total();
      }

      if (m_mpd) {
        auto song = m_mpd->get_song();

        if (song && song.get()) {
          artist = song->get_artist();
          album = song->get_album();
          title = song->get_title();
        }
      }
    } catch (const mpd_exception& err) {
      m_log.err(err.what());
      m_mpd.reset();
    }

    if (m_label_song) {
      m_label_song->reset_tokens();
      m_label_song->replace_token("%artist%", !artist.empty() ? artist : "untitled artist");
      m_label_song->replace_token("%album%", !album.empty() ? album : "untitled album");
      m_label_song->replace_token("%title%", !title.empty() ? title : "untitled track");
    }

    if (m_label_time) {
      m_label_time->reset_tokens();
      m_label_time->replace_token("%elapsed%", elapsed_str);
      m_label_time->replace_token("%total%", total_str);
    }

    if (m_icons->has("random"))
      m_icons->get("random")->m_foreground =
          m_status && m_status->random() ? m_toggle_on_color : m_toggle_off_color;
    if (m_icons->has("repeat"))
      m_icons->get("repeat")->m_foreground =
          m_status && m_status->repeat() ? m_toggle_on_color : m_toggle_off_color;
    if (m_icons->has("repeat_one"))
      m_icons->get("repeat_one")->m_foreground =
          m_status && m_status->single() ? m_toggle_on_color : m_toggle_off_color;

    return true;
  }

  string mpd_module::get_format() const {
    return connected() ? FORMAT_ONLINE : FORMAT_OFFLINE;
  }

  bool mpd_module::build(builder* builder, string tag) const {
    bool is_playing = false;
    bool is_paused = false;
    bool is_stopped = true;
    int elapsed_percentage = 0;

    if (m_status) {
      elapsed_percentage = m_status->get_elapsed_percentage();

      if (m_status->match_state(mpdstate::PLAYING))
        is_playing = true;
      if (m_status->match_state(mpdstate::PAUSED))
        is_paused = true;
      if (!(m_status->match_state(mpdstate::STOPPED)))
        is_stopped = false;
    }

    auto icon_cmd = [&builder](string cmd, icon_t icon) {
      builder->cmd(mousebtn::LEFT, cmd);
      builder->node(icon);
      builder->cmd_close();
    };

    if (tag == TAG_LABEL_SONG && !is_stopped)
      builder->node(m_label_song);
    else if (tag == TAG_LABEL_TIME && !is_stopped)
      builder->node(m_label_time);
    else if (tag == TAG_BAR_PROGRESS && !is_stopped)
      builder->node(m_bar_progress->output(elapsed_percentage));
    else if (tag == TAG_LABEL_OFFLINE)
      builder->node(m_label_offline);
    else if (tag == TAG_ICON_RANDOM)
      icon_cmd(EVENT_RANDOM, m_icons->get("random"));
    else if (tag == TAG_ICON_REPEAT)
      icon_cmd(EVENT_REPEAT, m_icons->get("repeat"));
    else if (tag == TAG_ICON_REPEAT_ONE)
      icon_cmd(EVENT_REPEAT_ONE, m_icons->get("repeat_one"));
    else if (tag == TAG_ICON_PREV)
      icon_cmd(EVENT_PREV, m_icons->get("prev"));
    else if (tag == TAG_ICON_STOP && (is_playing || is_paused))
      icon_cmd(EVENT_STOP, m_icons->get("stop"));
    else if ((tag == TAG_ICON_PAUSE || tag == TAG_TOGGLE) && is_playing)
      icon_cmd(EVENT_PAUSE, m_icons->get("pause"));
    else if ((tag == TAG_ICON_PLAY || tag == TAG_TOGGLE) && !is_playing)
      icon_cmd(EVENT_PLAY, m_icons->get("play"));
    else if (tag == TAG_ICON_NEXT)
      icon_cmd(EVENT_NEXT, m_icons->get("next"));
    else if (tag == TAG_ICON_SEEKB)
      icon_cmd(string(EVENT_SEEK).append("-5"), m_icons->get("seekb"));
    else if (tag == TAG_ICON_SEEKF)
      icon_cmd(string(EVENT_SEEK).append("+5"), m_icons->get("seekf"));
    else
      return false;
    return true;
  }

  bool mpd_module::handle_event(string cmd) {
    if (cmd.compare(0, 3, "mpd") != 0)
      return false;

    try {
      auto mpd = make_unique<mpdconnection>(m_log, m_host, m_port, m_pass);
      mpd->connect();

      auto status = mpd->get_status();

      if (cmd == EVENT_PLAY)
        mpd->play();
      else if (cmd == EVENT_PAUSE)
        mpd->pause(!(status->match_state(mpdstate::PAUSED)));
      else if (cmd == EVENT_STOP)
        mpd->stop();
      else if (cmd == EVENT_PREV)
        mpd->prev();
      else if (cmd == EVENT_NEXT)
        mpd->next();
      else if (cmd == EVENT_REPEAT_ONE)
        mpd->set_single(!status->single());
      else if (cmd == EVENT_REPEAT)
        mpd->set_repeat(!status->repeat());
      else if (cmd == EVENT_RANDOM)
        mpd->set_random(!status->random());
      else if (cmd.compare(0, strlen(EVENT_SEEK), EVENT_SEEK) == 0) {
        auto s = cmd.substr(strlen(EVENT_SEEK));
        int percentage = 0;
        if (s.empty())
          return false;
        if (s[0] == '+') {
          percentage = status->get_elapsed_percentage() + std::atoi(s.substr(1).c_str());
        } else if (s[0] == '-') {
          percentage = status->get_elapsed_percentage() - std::atoi(s.substr(1).c_str());
        } else {
          percentage = std::atoi(s.c_str());
        }
        mpd->seek(status->get_songid(), status->get_seek_position(percentage));
      } else
        return false;
    } catch (const mpd_exception& err) {
      m_log.err("%s: %s", name(), err.what());
      m_mpd.reset();
    }

    return true;
  }

  bool mpd_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END
