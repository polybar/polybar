#ifndef _MODULES_MPD_HPP_
#define _MODULES_MPD_HPP_

#include "modules/base.hpp"
#include "interfaces/mpd.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(MpdModule, EventModule)
  {
    static const int PROGRESSBAR_THREAD_SYNC_COUNT = 10;
    const std::chrono::duration<double> PROGRESSBAR_THREAD_INTERVAL = 1s;

    const char *FORMAT_ONLINE = "format:online";
    const char *TAG_BAR_PROGRESS = "<bar:progress>";
    const char *TAG_TOGGLE = "<toggle>";
    const char *TAG_LABEL_SONG = "<label:song>";
    const char *TAG_LABEL_TIME = "<label:time>";
    const char *TAG_ICON_RANDOM = "<icon:random>";
    const char *TAG_ICON_REPEAT = "<icon:repeat>";
    const char *TAG_ICON_REPEAT_ONE = "<icon:repeatone>";
    const char *TAG_ICON_PREV = "<icon:prev>";
    const char *TAG_ICON_STOP = "<icon:stop>";
    const char *TAG_ICON_PLAY = "<icon:play>";
    const char *TAG_ICON_PAUSE = "<icon:pause>";
    const char *TAG_ICON_NEXT = "<icon:next>";

    const char *FORMAT_OFFLINE = "format:offline";
    const char *TAG_LABEL_OFFLINE = "<label:offline>";

    const char *EVENT_PLAY = "mpdplay";
    const char *EVENT_PAUSE = "mpdpause";
    const char *EVENT_STOP = "mpdstop";
    const char *EVENT_PREV = "mpdprev";
    const char *EVENT_NEXT = "mpdnext";
    const char *EVENT_REPEAT = "mpdrepeat";
    const char *EVENT_REPEAT_ONE = "mpdrepeatone";
    const char *EVENT_RANDOM = "mpdrandom";
    const char *EVENT_SEEK = "mpdseek";

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

    std::shared_ptr<mpd::Connection> mpd;
    std::chrono::system_clock::time_point synced_at;
    float sync_interval;

    bool clickable_progress = false;
    std::string progress_fill, progress_empty, progress_indicator;

    public:
      MpdModule(const std::string& name);
      ~MpdModule();

      void start();
      bool has_event();
      bool update();
      std::string get_format();
      bool build(Builder *builder, const std::string& tag);

      bool handle_command(const std::string& cmd);
  };
}

#endif
