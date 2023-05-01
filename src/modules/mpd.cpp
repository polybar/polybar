#include "modules/mpd.hpp"

#include <csignal>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<mpd_module>;

  mpd_module::mpd_module(const bar_settings& bar, string name_, const config& config)
      : event_module<mpd_module>(bar, move(name_), config) {
    m_router->register_action(EVENT_PLAY, [this]() { action_play(); });
    m_router->register_action(EVENT_PAUSE, [this]() { action_pause(); });
    m_router->register_action(EVENT_STOP, [this]() { action_stop(); });
    m_router->register_action(EVENT_PREV, [this]() { action_prev(); });
    m_router->register_action(EVENT_NEXT, [this]() { action_next(); });
    m_router->register_action(EVENT_REPEAT, [this]() { action_repeat(); });
    m_router->register_action(EVENT_SINGLE, [this]() { action_single(); });
    m_router->register_action(EVENT_RANDOM, [this]() { action_random(); });
    m_router->register_action(EVENT_CONSUME, [this]() { action_consume(); });
    m_router->register_action_with_data(EVENT_SEEK, [this](const std::string& data) { action_seek(data); });

    m_host = m_conf.get(name(), "host", m_host);
    m_port = m_conf.get(name(), "port", m_port);
    m_pass = m_conf.get(name(), "password", m_pass);
    m_synctime = m_conf.get(name(), "interval", m_synctime);

    // Add formats and elements {{{
    auto format_online = m_conf.get<string>(name(), FORMAT_ONLINE, TAG_LABEL_SONG);
    for (auto&& format : {FORMAT_PLAYING, FORMAT_PAUSED, FORMAT_STOPPED}) {
      m_formatter->add(format, format_online,
          {TAG_BAR_PROGRESS, TAG_TOGGLE, TAG_TOGGLE_STOP, TAG_LABEL_SONG, TAG_LABEL_TIME, TAG_ICON_RANDOM,
              TAG_ICON_REPEAT, TAG_ICON_REPEAT_ONE, TAG_ICON_SINGLE, TAG_ICON_PREV, TAG_ICON_STOP, TAG_ICON_PLAY,
              TAG_ICON_PAUSE, TAG_ICON_NEXT, TAG_ICON_SEEKB, TAG_ICON_SEEKF, TAG_ICON_CONSUME});

      auto mod_format = m_formatter->get(format);

      mod_format->fg = m_conf.get(name(), FORMAT_ONLINE + "-foreground"s, mod_format->fg);
      mod_format->bg = m_conf.get(name(), FORMAT_ONLINE + "-background"s, mod_format->bg);
      mod_format->ul = m_conf.get(name(), FORMAT_ONLINE + "-underline"s, mod_format->ul);
      mod_format->ol = m_conf.get(name(), FORMAT_ONLINE + "-overline"s, mod_format->ol);
      mod_format->ulsize = m_conf.get(name(), FORMAT_ONLINE + "-underline-size"s, mod_format->ulsize);
      mod_format->olsize = m_conf.get(name(), FORMAT_ONLINE + "-overline-size"s, mod_format->olsize);
      mod_format->spacing = m_conf.get(name(), FORMAT_ONLINE + "-spacing"s, mod_format->spacing);
      mod_format->padding = m_conf.get(name(), FORMAT_ONLINE + "-padding"s, mod_format->padding);
      mod_format->margin = m_conf.get(name(), FORMAT_ONLINE + "-margin"s, mod_format->margin);
      mod_format->offset = m_conf.get(name(), FORMAT_ONLINE + "-offset"s, mod_format->offset);
      mod_format->font = m_conf.get(name(), FORMAT_ONLINE + "-font"s, mod_format->font);

      try {
        mod_format->prefix = load_label(m_conf, name(), FORMAT_ONLINE + "-prefix"s);
      } catch (const key_error& err) {
        // format-online-prefix not defined
      }

      try {
        mod_format->suffix = load_label(m_conf, name(), FORMAT_ONLINE + "-suffix"s);
      } catch (const key_error& err) {
        // format-online-suffix not defined
      }
    }

    m_formatter->add(FORMAT_OFFLINE, "", {TAG_LABEL_OFFLINE});

    m_icons = std::make_shared<iconset>();

    if (m_formatter->has(TAG_ICON_PLAY) || m_formatter->has(TAG_TOGGLE) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("play", load_label(m_conf, name(), TAG_ICON_PLAY));
    }
    if (m_formatter->has(TAG_ICON_PAUSE) || m_formatter->has(TAG_TOGGLE)) {
      m_icons->add("pause", load_label(m_conf, name(), TAG_ICON_PAUSE));
    }
    if (m_formatter->has(TAG_ICON_STOP) || m_formatter->has(TAG_TOGGLE_STOP)) {
      m_icons->add("stop", load_label(m_conf, name(), TAG_ICON_STOP));
    }
    if (m_formatter->has(TAG_ICON_PREV)) {
      m_icons->add("prev", load_label(m_conf, name(), TAG_ICON_PREV));
    }
    if (m_formatter->has(TAG_ICON_NEXT)) {
      m_icons->add("next", load_label(m_conf, name(), TAG_ICON_NEXT));
    }
    if (m_formatter->has(TAG_ICON_SEEKB)) {
      m_icons->add("seekb", load_label(m_conf, name(), TAG_ICON_SEEKB));
    }
    if (m_formatter->has(TAG_ICON_SEEKF)) {
      m_icons->add("seekf", load_label(m_conf, name(), TAG_ICON_SEEKF));
    }
    if (m_formatter->has(TAG_ICON_RANDOM)) {
      m_icons->add("random", load_label(m_conf, name(), TAG_ICON_RANDOM));
    }
    if (m_formatter->has(TAG_ICON_REPEAT)) {
      m_icons->add("repeat", load_label(m_conf, name(), TAG_ICON_REPEAT));
    }

    if (m_formatter->has(TAG_ICON_SINGLE)) {
      m_icons->add("single", load_label(m_conf, name(), TAG_ICON_SINGLE));
    } else if (m_formatter->has(TAG_ICON_REPEAT_ONE)) {
      m_conf.warn_deprecated(name(), "icon-repeatone", "icon-single");

      m_icons->add("single", load_label(m_conf, name(), TAG_ICON_REPEAT_ONE));
    }

    if (m_formatter->has(TAG_ICON_CONSUME)) {
      m_icons->add("consume", load_label(m_conf, name(), TAG_ICON_CONSUME));
    }

    if (m_formatter->has(TAG_LABEL_SONG)) {
      m_label_song = load_optional_label(m_conf, name(), TAG_LABEL_SONG, "%artist% - %title%");
    }
    if (m_formatter->has(TAG_LABEL_TIME)) {
      m_label_time = load_optional_label(m_conf, name(), TAG_LABEL_TIME, "%elapsed% / %total%");
    }
    if (m_formatter->has(TAG_ICON_RANDOM) || m_formatter->has(TAG_ICON_REPEAT) ||
        m_formatter->has(TAG_ICON_REPEAT_ONE) || m_formatter->has(TAG_ICON_SINGLE) ||
        m_formatter->has(TAG_ICON_CONSUME)) {
      m_toggle_on_color = m_conf.get(name(), "toggle-on-foreground", rgba{});
      m_toggle_off_color = m_conf.get(name(), "toggle-off-foreground", rgba{});
    }
    if (m_formatter->has(TAG_LABEL_OFFLINE, FORMAT_OFFLINE)) {
      m_label_offline = load_label(m_conf, name(), TAG_LABEL_OFFLINE);
    }
    if (m_formatter->has(TAG_BAR_PROGRESS)) {
      m_bar_progress = load_progressbar(m_bar, m_conf, name(), TAG_BAR_PROGRESS);
    }

    // }}}

    m_lastsync = chrono::steady_clock::now();

    try {
      m_mpd = std::make_unique<mpdconnection>(m_log, m_host, m_port, m_pass);
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
    if (connected()) {
      m_quick_attempts = 0;
      sleep(80ms);
    } else {
      sleep(m_quick_attempts++ < 5 ? 0.5s : 2s);
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
      if (!m_mpd) {
        m_mpd = std::make_unique<mpdconnection>(m_log, m_host, m_port, m_pass);
      }
      if (!connected()) {
        m_mpd->connect();
      }
    } catch (const mpd_exception& err) {
      m_log.err("%s: %s", name(), err.what());
      m_mpd.reset();
      return def;
    }

    if (!connected()) {
      return def;
    }

    if (!m_status) {
      m_status = m_mpd->get_status_safe();
    }

    try {
      m_mpd->idle();

      int idle_flags = 0;
      if ((idle_flags = m_mpd->noidle()) != 0) {
        // Update status on every event
        m_status->update(idle_flags, m_mpd.get());
        return true;
      }
    } catch (const mpd_exception& err) {
      m_log.err("%s: %s", name(), err.what());
      m_mpd.reset();
      return def;
    }

    if ((m_label_time || m_bar_progress) && m_status->match_state(mpdstate::PLAYING)) {
      auto now = chrono::steady_clock::now();
      auto diff = now - m_lastsync;

      if (chrono::duration_cast<chrono::milliseconds>(diff).count() > m_synctime * 1000) {
        m_lastsync = now;
        return true;
      }
    }

    return def;
  }

  bool mpd_module::update() {
    if (connected()) {
      m_statebroadcasted = mpd::connection_state::CONNECTED;
    } else if (!connected() && m_statebroadcasted != mpd::connection_state::DISCONNECTED) {
      m_statebroadcasted = mpd::connection_state::DISCONNECTED;
    } else if (!connected()) {
      return false;
    }

    if (!m_status) {
      if (connected() && (m_status = m_mpd->get_status_safe())) {
        return false;
      }
    }

    if (m_status && m_status->match_state(mpdstate::PLAYING)) {
      // Always update the status while playing
      m_status->update(-1, m_mpd.get());
    }

    string artist;
    string album_artist;
    string album;
    string title;
    string date;
    string elapsed_str;
    string total_str;

    try {
      if (m_status) {
        elapsed_str = m_status->get_formatted_elapsed();
        total_str = m_status->get_formatted_total();
      }

      if (m_mpd) {
        auto song = m_mpd->get_song();

        if (song) {
          artist = song->get_artist();
          album_artist = song->get_album_artist();
          album = song->get_album();
          title = song->get_title();
          date = song->get_date();
        }
      }
    } catch (const mpd_exception& err) {
      m_log.err("%s: %s", name(), err.what());
      m_mpd.reset();
    }

    if (m_label_song) {
      m_label_song->reset_tokens();
      m_label_song->replace_token("%artist%", !artist.empty() ? artist : "untitled artist");
      m_label_song->replace_token("%album-artist%", !album_artist.empty() ? album_artist : "untitled album artist");
      m_label_song->replace_token("%album%", !album.empty() ? album : "untitled album");
      m_label_song->replace_token("%title%", !title.empty() ? title : "untitled track");
      m_label_song->replace_token("%date%", !date.empty() ? date : "unknown date");
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
    if (m_icons->has("single")) {
      m_icons->get("single")->m_foreground = m_status && m_status->single() ? m_toggle_on_color : m_toggle_off_color;
    }
    if (m_icons->has("consume")) {
      m_icons->get("consume")->m_foreground = m_status && m_status->consume() ? m_toggle_on_color : m_toggle_off_color;
    }

    return true;
  }

  string mpd_module::get_format() const {
    if (!connected()) {
      return FORMAT_OFFLINE;
    } else if (m_status->match_state(mpdstate::PLAYING)) {
      return FORMAT_PLAYING;
    } else if (m_status->match_state(mpdstate::PAUSED)) {
      return FORMAT_PAUSED;
    } else {
      return FORMAT_STOPPED;
    }
  }

  string mpd_module::get_output() {
    if (m_status && m_status->get_queuelen() == 0) {
      m_log.info("%s: Hiding module since queue is empty", name());
      return "";
    } else {
      return event_module::get_output();
    }
  }

  bool mpd_module::build(builder* builder, const string& tag) const {
    bool is_playing = m_status && m_status->match_state(mpdstate::PLAYING);
    bool is_paused = m_status && m_status->match_state(mpdstate::PAUSED);
    bool is_stopped = m_status && m_status->match_state(mpdstate::STOPPED);

    if (tag == TAG_LABEL_SONG && !is_stopped) {
      builder->node(m_label_song);
    } else if (tag == TAG_LABEL_TIME && !is_stopped) {
      builder->node(m_label_time);
    } else if (tag == TAG_BAR_PROGRESS && !is_stopped) {
      builder->node(m_bar_progress->output(!m_status ? 0 : m_status->get_elapsed_percentage()));
    } else if (tag == TAG_LABEL_OFFLINE) {
      builder->node(m_label_offline);
    } else if (tag == TAG_ICON_RANDOM) {
      builder->action(mousebtn::LEFT, *this, EVENT_RANDOM, "", m_icons->get("random"));
    } else if (tag == TAG_ICON_REPEAT) {
      builder->action(mousebtn::LEFT, *this, EVENT_REPEAT, "", m_icons->get("repeat"));
    } else if (tag == TAG_ICON_REPEAT_ONE || tag == TAG_ICON_SINGLE) {
      builder->action(mousebtn::LEFT, *this, EVENT_SINGLE, "", m_icons->get("single"));
    } else if (tag == TAG_ICON_CONSUME) {
      builder->action(mousebtn::LEFT, *this, EVENT_CONSUME, "", m_icons->get("consume"));
    } else if (tag == TAG_ICON_PREV) {
      builder->action(mousebtn::LEFT, *this, EVENT_PREV, "", m_icons->get("prev"));
    } else if ((tag == TAG_ICON_STOP || tag == TAG_TOGGLE_STOP) && (is_playing || is_paused)) {
      builder->action(mousebtn::LEFT, *this, EVENT_STOP, "", m_icons->get("stop"));
    } else if ((tag == TAG_ICON_PAUSE || tag == TAG_TOGGLE) && is_playing) {
      builder->action(mousebtn::LEFT, *this, EVENT_PAUSE, "", m_icons->get("pause"));
    } else if ((tag == TAG_ICON_PLAY || tag == TAG_TOGGLE || tag == TAG_TOGGLE_STOP) && !is_playing) {
      builder->action(mousebtn::LEFT, *this, EVENT_PLAY, "", m_icons->get("play"));
    } else if (tag == TAG_ICON_NEXT) {
      builder->action(mousebtn::LEFT, *this, EVENT_NEXT, "", m_icons->get("next"));
    } else if (tag == TAG_ICON_SEEKB) {
      builder->action(mousebtn::LEFT, *this, EVENT_SEEK, "-5"s, m_icons->get("seekb"));
    } else if (tag == TAG_ICON_SEEKF) {
      builder->action(mousebtn::LEFT, *this, EVENT_SEEK, "+5"s, m_icons->get("seekf"));
    } else {
      return false;
    }

    return true;
  }

/**
 * Small macro to create a temporary mpd connection for the action handlers.
 *
 * We have to create a separate mpd instance because actions run in the
 * controller thread and the `m_mpd` pointer is used in the module thread.
 */
#define MPD_CONNECT()                                                        \
  auto mpd = std::make_unique<mpdconnection>(m_log, m_host, m_port, m_pass); \
  mpd->connect();                                                            \
  auto status = mpd->get_status()

  void mpd_module::action_play() {
    MPD_CONNECT();
    if (!status->match_state(mpdstate::PLAYING)) {
      mpd->play();
    }
  }

  void mpd_module::action_pause() {
    MPD_CONNECT();

    if (!status->match_state(mpdstate::PAUSED)) {
      mpd->pause(true);
    }
  }

  void mpd_module::action_stop() {
    MPD_CONNECT();

    if (!status->match_state(mpdstate::STOPPED)) {
      mpd->stop();
    }
  }

  void mpd_module::action_prev() {
    MPD_CONNECT();

    if (!status->match_state(mpdstate::STOPPED)) {
      mpd->prev();
    }
  }

  void mpd_module::action_next() {
    MPD_CONNECT();

    if (!status->match_state(mpdstate::STOPPED)) {
      mpd->next();
    }
  }

  void mpd_module::action_repeat() {
    MPD_CONNECT();
    mpd->set_repeat(!status->repeat());
  }

  void mpd_module::action_single() {
    MPD_CONNECT();
    mpd->set_single(!status->single());
  }

  void mpd_module::action_random() {
    MPD_CONNECT();
    mpd->set_random(!status->random());
  }

  void mpd_module::action_consume() {
    MPD_CONNECT();
    mpd->set_consume(!status->consume());
  }

  void mpd_module::action_seek(const string& data) {
    MPD_CONNECT();

    int percentage = 0;
    if (data.empty()) {
      return;
    } else if (data[0] == '+') {
      percentage = status->get_elapsed_percentage() + std::strtol(data.substr(1).c_str(), nullptr, 10);
    } else if (data[0] == '-') {
      percentage = status->get_elapsed_percentage() - std::strtol(data.substr(1).c_str(), nullptr, 10);
    } else {
      percentage = std::strtol(data.c_str(), nullptr, 10);
    }
    mpd->seek(status->get_songid(), status->get_seek_position(percentage));
  }

#undef MPD_CONNECT
}  // namespace modules

POLYBAR_NS_END
