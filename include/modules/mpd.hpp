#pragma once

#include "modules/base.hpp"
#include "interfaces/mpd.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(MpdModule, EventModule)
  {
    std::string mpd_host = "127.0.0.1";
    std::string mpd_pass = "";
    int mpd_port = 6600;

    static const int PROGRESSBAR_THREAD_SYNC_COUNT = 10;
    const std::chrono::duration<double> PROGRESSBAR_THREAD_INTERVAL = 1s;

    static constexpr auto FORMAT_ONLINE = "format-online";
    static constexpr auto TAG_BAR_PROGRESS = "<bar-progress>";
    static constexpr auto TAG_TOGGLE = "<toggle>";
    static constexpr auto TAG_LABEL_SONG = "<label-song>";
    static constexpr auto TAG_LABEL_TIME = "<label-time>";
    static constexpr auto TAG_ICON_RANDOM = "<icon-random>";
    static constexpr auto TAG_ICON_REPEAT = "<icon-repeat>";
    static constexpr auto TAG_ICON_REPEAT_ONE = "<icon-repeatone>";
    static constexpr auto TAG_ICON_PREV = "<icon-prev>";
    static constexpr auto TAG_ICON_STOP = "<icon-stop>";
    static constexpr auto TAG_ICON_PLAY = "<icon-play>";
    static constexpr auto TAG_ICON_PAUSE = "<icon-pause>";
    static constexpr auto TAG_ICON_NEXT = "<icon-next>";

    static constexpr auto FORMAT_OFFLINE = "format-offline";
    static constexpr auto TAG_LABEL_OFFLINE = "<label-offline>";

    static constexpr auto EVENT_PLAY = "mpdplay";
    static constexpr auto EVENT_PAUSE = "mpdpause";
    static constexpr auto EVENT_STOP = "mpdstop";
    static constexpr auto EVENT_PREV = "mpdprev";
    static constexpr auto EVENT_NEXT = "mpdnext";
    static constexpr auto EVENT_REPEAT = "mpdrepeat";
    static constexpr auto EVENT_REPEAT_ONE = "mpdrepeatone";
    static constexpr auto EVENT_RANDOM = "mpdrandom";
    static constexpr auto EVENT_SEEK = "mpdseek";

    std::unique_ptr<drawtypes::Bar> bar_progress;
    std::unique_ptr<drawtypes::IconMap> icons;
    std::unique_ptr<drawtypes::Label> label_song;
    std::unique_ptr<drawtypes::Label> label_song_tokenized;
    std::unique_ptr<drawtypes::Label> label_time;
    std::unique_ptr<drawtypes::Label> label_time_tokenized;
    std::unique_ptr<drawtypes::Label> label_offline;

    std::unique_ptr<mpd::Status> status;

    std::string toggle_on_color;
    std::string toggle_off_color;

    std::unique_ptr<mpd::Connection> mpd;
    std::chrono::system_clock::time_point synced_at;
    float sync_interval = 0.5f;

    bool clickable_progress = false;
    std::string progress_fill, progress_empty, progress_indicator;

    public:
      explicit MpdModule(const std::string& name);
      ~MpdModule();

      void start();
      bool has_event();
      bool update();
      std::string get_format();
      bool build(Builder *builder, const std::string& tag);

      bool handle_command(const std::string& cmd);
  };
}
