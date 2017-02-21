#include "modules/mpris.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "utils/factory.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<mpris_module>;

  mpris_module::mpris_module(const bar_settings& bar, string name_) : event_module<mpris_module>(bar, move(name_)) {
    m_player = m_conf.get(name(), "player", m_player);
    m_synctime = m_conf.get(name(), "interval", m_synctime);

    // Add formats and elements {{{

    m_formatter->add(FORMAT_ONLINE, TAG_LABEL_SONG,
        {TAG_BAR_PROGRESS, TAG_TOGGLE, TAG_TOGGLE_STOP, TAG_LABEL_SONG, TAG_LABEL_TIME, TAG_ICON_RANDOM,
            TAG_ICON_REPEAT, TAG_ICON_REPEAT_ONE, TAG_ICON_PREV, TAG_ICON_STOP, TAG_ICON_PLAY, TAG_ICON_PAUSE,
            TAG_ICON_NEXT, TAG_ICON_SEEKB, TAG_ICON_SEEKF});

    m_formatter->add(FORMAT_OFFLINE, "", {TAG_LABEL_OFFLINE});

    m_icons = factory_util::shared<iconset>();

    if (m_formatter->has(TAG_ICON_PLAY) || m_formatter->has(TAG_TOGGLE) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("play", load_icon(m_conf, name(), TAG_ICON_PLAY));
    }
    if (m_formatter->has(TAG_ICON_PAUSE) || m_formatter->has(TAG_TOGGLE)) {
      m_icons->add("pause", load_icon(m_conf, name(), TAG_ICON_PAUSE));
    }
    if (m_formatter->has(TAG_ICON_STOP) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("stop", load_icon(m_conf, name(), TAG_ICON_STOP));
    }
    if (m_formatter->has(TAG_ICON_PREV)) {
      m_icons->add("prev", load_icon(m_conf, name(), TAG_ICON_PREV));
    }
    if (m_formatter->has(TAG_ICON_NEXT)) {
      m_icons->add("next", load_icon(m_conf, name(), TAG_ICON_NEXT));
    }
    if (m_formatter->has(TAG_ICON_SEEKB)) {
      m_icons->add("seekb", load_icon(m_conf, name(), TAG_ICON_SEEKB));
    }
    if (m_formatter->has(TAG_ICON_SEEKF)) {
      m_icons->add("seekf", load_icon(m_conf, name(), TAG_ICON_SEEKF));
    }
    if (m_formatter->has(TAG_ICON_RANDOM)) {
      m_icons->add("random", load_icon(m_conf, name(), TAG_ICON_RANDOM));
    }
    if (m_formatter->has(TAG_ICON_REPEAT)) {
      m_icons->add("repeat", load_icon(m_conf, name(), TAG_ICON_REPEAT));
    }
    if (m_formatter->has(TAG_ICON_REPEAT_ONE)) {
      m_icons->add("repeat_one", load_icon(m_conf, name(), TAG_ICON_REPEAT_ONE));
    }

    if (m_formatter->has(TAG_LABEL_SONG)) {
      m_label_song = load_optional_label(m_conf, name(), TAG_LABEL_SONG, "%artist% - %title%");
    }
    if (m_formatter->has(TAG_LABEL_TIME)) {
      m_label_time = load_optional_label(m_conf, name(), TAG_LABEL_TIME, "%elapsed% / %total%");
    }
    if (m_formatter->has(TAG_ICON_RANDOM) || m_formatter->has(TAG_ICON_REPEAT) ||
        m_formatter->has(TAG_ICON_REPEAT_ONE)) {
      m_toggle_on_color = m_conf.get(name(), "toggle-on-foreground", ""s);
      m_toggle_off_color = m_conf.get(name(), "toggle-off-foreground", ""s);
    }
    if (m_formatter->has(TAG_LABEL_OFFLINE, FORMAT_OFFLINE)) {
      m_label_offline = load_label(m_conf, name(), TAG_LABEL_OFFLINE);
    }
    if (m_formatter->has(TAG_BAR_PROGRESS)) {
      m_bar_progress = load_progressbar(m_bar, m_conf, name(), TAG_BAR_PROGRESS);
    }

    // }}}

    m_lastsync = chrono::system_clock::now();
    m_connection = factory_util::unique<mprisconnection>(m_log, m_player);
  }

  inline bool mpris_module::connected() const {
    return m_connection->connected();
  }

  bool mpris_module::has_event() {
    if (!connected() && m_status == nullptr) {
      return false;
    } else if (!connected()) {
      return true;
    } else if (m_status == nullptr) {
      m_status = m_connection->get_status();
      return true;
    }

    auto new_status = m_connection->get_status();
    auto new_song = m_connection->get_song();

    if (m_song != new_song) {
      m_song = new_song;
      m_status = std::move(new_status);
      return true;
    }

    if (new_status == nullptr || m_status->playback_status != new_status->playback_status) {
      m_song = new_song;
      m_status = std::move(new_status);
      return true;
    }

    if ((m_label_time || m_bar_progress) && m_connection->connected()) {
      auto now = chrono::system_clock::now();
      auto diff = now - m_lastsync;

      if (chrono::duration_cast<chrono::milliseconds>(diff).count() > m_synctime * 1000) {
        m_lastsync = now;
        return true;
      }
    }

    return false;
  }

  bool mpris_module::update() {
    if (!m_connection->connected() && m_status == nullptr) {
//      return true;
      return false;
    } else if (!m_connection->connected()) {
      m_status = nullptr;
      return true;
    }

    string artist;
    string album;
    string title;
    string date;
    string elapsed_str;
    string total_str;

    /*
        elapsed_str = m_status->get_formatted_elapsed();
        total_str = m_status->get_formatted_total();
*/

    m_song = m_connection->get_song();
    //auto status = m_connection->get_status();

    title = m_song.get_title();
    artist = m_song.get_artist();
    album = m_song.get_album();

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

    if (m_icons->has("random")) {
      m_icons->get("random")->m_foreground = m_status && m_status->random() ? m_toggle_on_color : m_toggle_off_color;
    }
    if (m_icons->has("repeat")) {
      m_icons->get("repeat")->m_foreground = m_status && m_status->repeat() ? m_toggle_on_color : m_toggle_off_color;
    }
    if (m_icons->has("repeat_one")) {
      m_icons->get("repeat_one")->m_foreground =
          m_status && m_status->single() ? m_toggle_on_color : m_toggle_off_color;
    }

    return true;
  }

  string mpris_module::get_format() const {
    return connected() ? FORMAT_ONLINE : FORMAT_OFFLINE;
  }

  bool mpris_module::build(builder* builder, const string& tag) const {
    bool is_playing = m_status && m_status->playback_status == "Playing";
    bool is_paused = m_status && m_status->playback_status == "Paused";
    bool is_stopped = m_status && m_status->playback_status == "Stopped";

    if (tag == TAG_LABEL_SONG && !is_stopped) {
      builder->node(m_label_song);
    } else if (tag == TAG_LABEL_TIME && !is_stopped) {
      builder->node(m_label_time);
    } else if (tag == TAG_BAR_PROGRESS && !is_stopped) {
      //      builder->node(m_bar_progress->output(!m_status ? 0 : m_status->get_elapsed_percentage()));
      builder->node(m_bar_progress->output(!m_status ? 0 : 20));
    } else if (tag == TAG_LABEL_OFFLINE) {
      builder->node(m_label_offline);
    } else if (tag == TAG_ICON_RANDOM) {
      builder->cmd(mousebtn::LEFT, EVENT_RANDOM, m_icons->get("random"));
    } else if (tag == TAG_ICON_REPEAT) {
      builder->cmd(mousebtn::LEFT, EVENT_REPEAT, m_icons->get("repeat"));
    } else if (tag == TAG_ICON_REPEAT_ONE) {
      builder->cmd(mousebtn::LEFT, EVENT_REPEAT_ONE, m_icons->get("repeat_one"));
    } else if (tag == TAG_ICON_PREV) {
      builder->cmd(mousebtn::LEFT, EVENT_PREV, m_icons->get("prev"));
    } else if ((tag == TAG_ICON_STOP || tag == TAG_TOGGLE_STOP) && (is_playing || is_paused)) {
      builder->cmd(mousebtn::LEFT, EVENT_STOP, m_icons->get("stop"));
    } else if ((tag == TAG_ICON_PAUSE || tag == TAG_TOGGLE) && is_playing) {
      builder->cmd(mousebtn::LEFT, EVENT_PAUSE, m_icons->get("pause"));
    } else if ((tag == TAG_ICON_PLAY || tag == TAG_TOGGLE || tag == TAG_TOGGLE_STOP) && !is_playing) {
      builder->cmd(mousebtn::LEFT, EVENT_PLAY, m_icons->get("play"));
    } else if (tag == TAG_ICON_NEXT) {
      builder->cmd(mousebtn::LEFT, EVENT_NEXT, m_icons->get("next"));
    } else if (tag == TAG_ICON_SEEKB) {
      builder->cmd(mousebtn::LEFT, EVENT_SEEK + "-5"s, m_icons->get("seekb"));
    } else if (tag == TAG_ICON_SEEKF) {
      builder->cmd(mousebtn::LEFT, EVENT_SEEK + "+5"s, m_icons->get("seekf"));
    } else {
      return false;
    }

    return true;
  }

  bool mpris_module::input(string&& cmd) {
    if (cmd.compare(0, 5, "mpris") != 0) {
      return false;
    }

    if (cmd == EVENT_PLAY) {
      m_connection->play();
    } else if (cmd == EVENT_PAUSE) {
      m_connection->pause();
    } else if (cmd == EVENT_STOP) {
      m_connection->stop();
    } else if (cmd == EVENT_PREV) {
      m_connection->prev();
    } else if (cmd == EVENT_NEXT) {
      m_connection->next();
    } else if (cmd == EVENT_REPEAT_ONE) {
      // mpd->set_single(!status->single());
    } else if (cmd == EVENT_REPEAT) {
      // mpd->set_repeat(!status->repeat());
    } else if (cmd == EVENT_RANDOM) {
      // mpd->set_random(!status->random());
        /*
    } else if (cmd.compare(0, strlen(EVENT_SEEK), EVENT_SEEK) == 0) {
      auto s = cmd.substr(strlen(EVENT_SEEK));
      int percentage = 0;
      if (s.empty()) {
        return false;
      } else if (s[0] == '+') {
        //    percentage = status->get_elapsed_percentage() + std::atoi(s.substr(1).c_str());
      } else if (s[0] == '-') {
        //    percentage = status->get_elapsed_percentage() - std::atoi(s.substr(1).c_str());
      } else {
        percentage = std::atoi(s.c_str());
      }
      //    mpd->seek(status->get_songid(), status->get_seek_position(percentage));
      //    */
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
