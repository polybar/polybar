#pragma once

#include <csignal>

#include "adapters/mpd.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "modules/meta.hpp"
#include "utils/threading.hpp"

LEMONBUDDY_NS

using namespace mpd;

namespace modules {
  class mpd_module : public event_module<mpd_module> {
   public:
    using event_module::event_module;

    void setup();
    void teardown();
    inline bool connected() const;
    void idle();
    bool has_event();
    bool update();
    string get_format() const;
    bool build(builder* builder, string tag) const;
    bool handle_event(string cmd);
    bool receive_events() const;

   private:
    // static const int PROGRESSBAR_THREAD_SYNC_COUNT = 10;
    // const chrono::duration<double> PROGRESSBAR_THREAD_INTERVAL = 1s;

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
    static constexpr auto TAG_ICON_SEEKB = "<icon-seekb>";
    static constexpr auto TAG_ICON_SEEKF = "<icon-seekf>";

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

    progressbar_t m_bar_progress;
    iconset_t m_icons;
    label_t m_label_song;
    label_t m_label_time;
    label_t m_label_offline;

    unique_ptr<mpdconnection> m_mpd;
    unique_ptr<mpdstatus> m_status;

    string m_host = "127.0.0.1";
    string m_pass = "";
    unsigned int m_port = 6600;

    string m_toggle_on_color;
    string m_toggle_off_color;

    chrono::system_clock::time_point m_lastsync;
    float m_synctime = 1.0f;

    string m_progress_fill;
    string m_progress_empty;
    string m_progress_indicator;

    // This flag is used to let thru a broadcast once every time
    // the connection state changes
    mpd::connection_state m_statebroadcasted;
  };
}

LEMONBUDDY_NS_END
