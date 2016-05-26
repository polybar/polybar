#include <thread>

#include "bar.hpp"
#include "lemonbuddy.hpp"
#include "modules/mpd.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"
#include "utils/memory.hpp"

using namespace modules;
using namespace mpd;

MpdModule::MpdModule(const std::string& name_)
  : EventModule(name_), icons(std::make_unique<drawtypes::IconMap>())
{
  // Load configuration values {{{
  this->mpd_host = config::get<std::string>(this->name(), "host", "127.0.0.1");
  this->mpd_port = config::get<int>(this->name(), "port", 6600);
  this->mpd_pass = config::get<std::string>(this->name(), "password", "");
  // }}}

  // Add formats and elements {{{
  this->formatter->add(FORMAT_ONLINE, TAG_LABEL_SONG, {
    TAG_BAR_PROGRESS, TAG_TOGGLE,      TAG_LABEL_SONG,      TAG_LABEL_TIME,
    TAG_ICON_RANDOM,  TAG_ICON_REPEAT, TAG_ICON_REPEAT_ONE, TAG_ICON_PREV,
    TAG_ICON_STOP,    TAG_ICON_PLAY,   TAG_ICON_PAUSE,      TAG_ICON_NEXT });
  this->formatter->add(FORMAT_OFFLINE, "", { TAG_LABEL_OFFLINE });

  if (this->formatter->has(TAG_ICON_PLAY) || this->formatter->has(TAG_TOGGLE))
    this->icons->add("play", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_PLAY)));
  if (this->formatter->has(TAG_ICON_PAUSE) || this->formatter->has(TAG_TOGGLE))
    this->icons->add("pause", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_PAUSE)));
  if (this->formatter->has(TAG_ICON_STOP))
    this->icons->add("stop", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_STOP)));
  if (this->formatter->has(TAG_ICON_PREV))
    this->icons->add("prev", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_PREV)));
  if (this->formatter->has(TAG_ICON_NEXT))
    this->icons->add("next", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_NEXT)));
  if (this->formatter->has(TAG_ICON_RANDOM))
    this->icons->add("random", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_RANDOM)));
  if (this->formatter->has(TAG_ICON_REPEAT))
    this->icons->add("repeat", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_REPEAT)));
  if (this->formatter->has(TAG_ICON_REPEAT_ONE))
    this->icons->add("repeat_one", drawtypes::get_config_icon(name(), get_tag_name(TAG_ICON_REPEAT_ONE)));

  if (this->formatter->has(TAG_LABEL_SONG)) {
    this->label_song = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_SONG), "%artist% - %title%");
    this->label_song_tokenized = this->label_song->clone();
  }
  if (this->formatter->has(TAG_LABEL_TIME)) {
    this->label_time = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_TIME), "%elapsed% / %total%");
    this->label_time_tokenized = this->label_time->clone();
  }
  if (this->formatter->has(TAG_ICON_RANDOM) || this->formatter->has(TAG_ICON_REPEAT) || this->formatter->has(TAG_ICON_REPEAT_ONE)) {
    this->toggle_on_color = config::get<std::string>(name(), "toggle_on-foreground", "");
    this->toggle_off_color = config::get<std::string>(name(), "toggle_off-foreground", "");
  }
  if (this->formatter->has(TAG_LABEL_OFFLINE, FORMAT_OFFLINE))
    this->label_offline = drawtypes::get_config_label(name(), get_tag_name(TAG_LABEL_OFFLINE));
  if (this->formatter->has(TAG_BAR_PROGRESS)) {
    this->bar_progress = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_PROGRESS));
  }
  // }}}

  // Sign up for stdin events {{{
  register_command_handler(name());
  // }}}
}

MpdModule::~MpdModule()
{
  std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
  this->status.reset();
}

void MpdModule::start()
{
  this->mpd = std::make_unique<mpd::Connection>(this->mpd_host, this->mpd_port, this->mpd_pass);

  this->synced_at = std::chrono::system_clock::now();
  this->sync_interval = config::get<float>(name(), "interval", this->sync_interval) * 1000;

  try {
    this->mpd->connect();
    this->status = this->mpd->get_status();
    this->status->update(-1, this->mpd);
  } catch (mpd::Exception &e) {
    log_error(e.what());
    this->mpd->disconnect();
  }

  this->EventModule::start();
}

bool MpdModule::has_event()
{
  bool has_event = false;

  if (!this->mpd->connected()) {
    try {
      this->mpd->connect();
    } catch (mpd::Exception &e) {
      get_logger()->debug(e.what());
    }

    if (!this->mpd->connected()) {
      std::this_thread::sleep_for(3s);
      return false;
    }
  }

  if (!this->status) {
    this->status = this->mpd->get_status();
    this->status->update(-1, this->mpd);
  }

  try {
    this->mpd->idle();

    int idle_flags;

    if ((idle_flags = this->mpd->noidle()) != 0) {
      this->status->update(idle_flags, this->mpd);
      has_event = true;
    } else if (this->status->state & mpd::PLAYING) {
      this->status->update_timer();
    }
  } catch (mpd::Exception &e) {
    log_error(e.what());
    this->mpd->disconnect();
    has_event = true;
  }

  if (this->label_time || this->bar_progress) {
    auto now = std::chrono::system_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - this->synced_at).count() > this->sync_interval) {
      has_event = true;
      this->synced_at = now;
    }
  }

  return has_event;
}

bool MpdModule::update()
{
  if (!this->mpd->connected())
    return true;

  if (!this->status)
    try {
      this->status = this->mpd->get_status();
    } catch (mpd::Exception &e) {
      log_trace(e.what());
    }

  if (!this->status)
    return true;

  std::unique_ptr<mpd::Song> song;
  std::string artist, album, title, elapsed_str, total_str;

  try {
    elapsed_str = this->status->get_formatted_elapsed();
    total_str = this->status->get_formatted_total();

    song = this->mpd->get_song();

    if (*song) {
      artist = song->get_artist();
      album = song->get_album();
      title = song->get_title();
    }
  } catch (mpd::Exception &e) {
    log_error(e.what());
    this->mpd->disconnect();
    return true;
  }

  if (this->label_song) {
    this->label_song_tokenized->text = this->label_song->text;
    this->label_song_tokenized->replace_token("%artist%", artist);
    this->label_song_tokenized->replace_token("%album%", album);
    this->label_song_tokenized->replace_token("%title%", title);
  }

  if (this->label_time) {
    this->label_time_tokenized->text = this->label_time->text;
    this->label_time_tokenized->replace_token("%elapsed%", elapsed_str);
    this->label_time_tokenized->replace_token("%total%", total_str);
  }

  if (this->icons->has("random"))
    this->icons->get("random")->fg = this->status->random ? this->toggle_on_color : this->toggle_off_color;
  if (this->icons->has("repeat"))
    this->icons->get("repeat")->fg = this->status->repeat ? this->toggle_on_color : this->toggle_off_color;
  if (this->icons->has("repeat_one"))
    this->icons->get("repeat_one")->fg = this->status->single ? this->toggle_on_color : this->toggle_off_color;

  return true;
}

std::string MpdModule::get_format() {
  return this->mpd->connected() ? FORMAT_ONLINE : FORMAT_OFFLINE;
}

bool MpdModule::build(Builder *builder, const std::string& tag)
{
  auto icon_cmd = [](Builder *builder, std::string cmd, std::unique_ptr<drawtypes::Icon> &icon){
    builder->cmd(Cmd::LEFT_CLICK, cmd);
      builder->node(icon);
    builder->cmd_close();
  };

  bool is_playing = false;
  bool is_stopped = true;
  int elapsed_percentage = 0;

  if (this->status) {
    elapsed_percentage = this->status->get_elapsed_percentage();

    if (this->status->state & mpd::State::PLAYING)
      is_playing = true;
    if (!(this->status->state & mpd::State::STOPPED))
      is_stopped = false;
  }

  if (tag == TAG_LABEL_SONG && !is_stopped)
    builder->node(this->label_song_tokenized);
  else if (tag == TAG_LABEL_TIME && !is_stopped)
    builder->node(this->label_time_tokenized);
  else if (tag == TAG_BAR_PROGRESS && !is_stopped)
    builder->node(this->bar_progress, elapsed_percentage);
  else if (tag == TAG_LABEL_OFFLINE)
    builder->node(this->label_offline);
  else if (tag == TAG_ICON_RANDOM)
    icon_cmd(builder, EVENT_RANDOM, this->icons->get("random"));
  else if (tag == TAG_ICON_REPEAT)
    icon_cmd(builder, EVENT_REPEAT, this->icons->get("repeat"));
  else if (tag == TAG_ICON_REPEAT_ONE)
    icon_cmd(builder, EVENT_REPEAT_ONE, this->icons->get("repeat_one"));
  else if (tag == TAG_ICON_PREV)
    icon_cmd(builder, EVENT_PREV, this->icons->get("prev"));
  else if (tag == TAG_ICON_STOP)
    icon_cmd(builder, EVENT_STOP, this->icons->get("stop"));
  else if (tag == TAG_ICON_PAUSE || (tag == TAG_TOGGLE && is_playing))
    icon_cmd(builder, EVENT_PAUSE, this->icons->get("pause"));
  else if (tag == TAG_ICON_PLAY || (tag == TAG_TOGGLE && !is_playing))
    icon_cmd(builder, EVENT_PLAY, this->icons->get("play"));
  else if (tag == TAG_ICON_NEXT)
    icon_cmd(builder, EVENT_NEXT, this->icons->get("next"));
  else
    return false;

  return true;
}

bool MpdModule::handle_command(const std::string& cmd)
{
  if (cmd.length() < 3 || cmd.substr(0, 3) != "mpd")
    return false;

  try {
    auto mpd = std::make_unique<mpd::Connection>(this->mpd_host, this->mpd_port, this->mpd_pass);

    mpd->connect();

    if (cmd == EVENT_PLAY)
      mpd->play();
    else if (cmd == EVENT_PAUSE)
      if (this->status && this->status.get())
        mpd->pause(!(this->status->state & mpd::State::PAUSED));
      else
        mpd->pause(true);
    else if (cmd == EVENT_STOP)
      mpd->stop();
    else if (cmd == EVENT_PREV)
      mpd->prev();
    else if (cmd == EVENT_NEXT)
      mpd->next();
    else if (cmd == EVENT_REPEAT_ONE)
      if (this->status)
        mpd->set_single(!this->status->single);
      else
        mpd->set_single(true);
    else if (cmd == EVENT_REPEAT)
      if (this->status)
        mpd->set_repeat(!this->status->repeat);
      else
        mpd->set_repeat(true);
    else if (cmd == EVENT_RANDOM)
      if (this->status)
        mpd->set_random(!this->status->random);
      else
        mpd->set_random(true);
    else if (cmd.compare(0, std::strlen(EVENT_SEEK), EVENT_SEEK) == 0) {
      auto s = cmd.substr(std::strlen(EVENT_SEEK));
      int perc = 0;
      if (s.empty())
        return false;
      if (s[0] == '+') {
        perc = this->status->get_elapsed_percentage()
          + std::atoi(s.substr(1).c_str());
      } else if (s[0] == '-') {
        perc = this->status->get_elapsed_percentage()
          - std::atoi(s.substr(1).c_str());
      } else {
        perc = std::atoi(s.c_str());
      }
      mpd->seek(perc);
    } else
      return false;
  } catch (mpd::Exception &e) {
    log_error(e.what());
  }

  return true;
}
