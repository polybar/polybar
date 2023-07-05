#pragma once

#include <chrono>

#include "adapters/mpd.hpp"
#include "modules/meta/event_module.hpp"
#include "modules/meta/types.hpp"
#include "utils/env.hpp"

POLYBAR_NS

using namespace mpd;

namespace modules {
  class mpd_module : public event_module<mpd_module> {
   public:
    explicit mpd_module(const bar_settings&, string, const config&);

    void teardown();
    inline bool connected() const;
    void idle();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = MPD_TYPE;

    static constexpr const char* EVENT_PLAY = "play";
    static constexpr const char* EVENT_PAUSE = "pause";
    static constexpr const char* EVENT_STOP = "stop";
    static constexpr const char* EVENT_PREV = "prev";
    static constexpr const char* EVENT_NEXT = "next";
    static constexpr const char* EVENT_REPEAT = "repeat";
    static constexpr const char* EVENT_SINGLE = "single";
    static constexpr const char* EVENT_RANDOM = "random";
    static constexpr const char* EVENT_CONSUME = "consume";
    static constexpr const char* EVENT_SEEK = "seek";

   private:
    void action_play();
    void action_pause();
    void action_stop();
    void action_prev();
    void action_next();
    void action_repeat();
    void action_single();
    void action_random();
    void action_consume();
    void action_seek(const string& data);

   private:
#define DEF_BAR_PROGRESS "bar-progress"
#define DEF_LABEL_SONG "label-song"
#define DEF_LABEL_TIME "label-time"
#define DEF_ICON_RANDOM "icon-random"
#define DEF_ICON_REPEAT "icon-repeat"
#define DEF_ICON_SINGLE "icon-single"
#define DEF_ICON_CONSUME "icon-consume"
#define DEF_ICON_PREV "icon-prev"
#define DEF_ICON_STOP "icon-stop"
#define DEF_ICON_PLAY "icon-play"
#define DEF_ICON_PAUSE "icon-pause"
#define DEF_ICON_NEXT "icon-next"
#define DEF_ICON_SEEKB "icon-seekb"
#define DEF_ICON_SEEKF "icon-seekf"
#define DEF_LABEL_OFFLINE "label-offline"
#define DEF_ICON_REPEAT_ONE "icon-repeatone"

    static constexpr const char* FORMAT_ONLINE{"format-online"};
    static constexpr const char* FORMAT_PLAYING{"format-playing"};
    static constexpr const char* FORMAT_PAUSED{"format-paused"};
    static constexpr const char* FORMAT_STOPPED{"format-stopped"};
    static constexpr const char* NAME_BAR_PROGRESS{DEF_BAR_PROGRESS};
    static constexpr const char* TAG_BAR_PROGRESS{"<" DEF_BAR_PROGRESS ">"};
    static constexpr const char* TAG_TOGGLE{"<toggle>"};
    static constexpr const char* TAG_TOGGLE_STOP{"<toggle-stop>"};
    static constexpr const char* NAME_LABEL_SONG{DEF_LABEL_SONG};
    static constexpr const char* TAG_LABEL_SONG{"<" DEF_LABEL_SONG ">"};
    static constexpr const char* NAME_LABEL_TIME{DEF_LABEL_TIME};
    static constexpr const char* TAG_LABEL_TIME{"<" DEF_LABEL_TIME ">"};
    static constexpr const char* NAME_ICON_RANDOM{DEF_ICON_RANDOM};
    static constexpr const char* TAG_ICON_RANDOM{"<" DEF_ICON_RANDOM ">"};
    static constexpr const char* NAME_ICON_REPEAT{DEF_ICON_REPEAT};
    static constexpr const char* TAG_ICON_REPEAT{"<" DEF_ICON_REPEAT ">"};
    /*
     * Deprecated
     */
    static constexpr const char* NAME_ICON_REPEAT_ONE{DEF_ICON_REPEAT_ONE};
    static constexpr const char* TAG_ICON_REPEAT_ONE{"<" DEF_ICON_REPEAT_ONE ">"};
    /*
     * Replaces icon-repeatone
     *
     * repeatone is misleading, since it doesn't actually affect the repeating behaviour
     */
    static constexpr const char* NAME_ICON_SINGLE{DEF_ICON_SINGLE};
    static constexpr const char* TAG_ICON_SINGLE{"<" DEF_ICON_SINGLE ">"};
    static constexpr const char* NAME_ICON_CONSUME{DEF_ICON_CONSUME};
    static constexpr const char* TAG_ICON_CONSUME{"<" DEF_ICON_CONSUME ">"};
    static constexpr const char* NAME_ICON_PREV{DEF_ICON_PREV};
    static constexpr const char* TAG_ICON_PREV{"<" DEF_ICON_PREV ">"};
    static constexpr const char* NAME_ICON_STOP{DEF_ICON_STOP};
    static constexpr const char* TAG_ICON_STOP{"<" DEF_ICON_STOP ">"};
    static constexpr const char* NAME_ICON_PLAY{DEF_ICON_PLAY};
    static constexpr const char* TAG_ICON_PLAY{"<" DEF_ICON_PLAY ">"};
    static constexpr const char* NAME_ICON_PAUSE{DEF_ICON_PAUSE};
    static constexpr const char* TAG_ICON_PAUSE{"<" DEF_ICON_PAUSE ">"};
    static constexpr const char* NAME_ICON_NEXT{DEF_ICON_NEXT};
    static constexpr const char* TAG_ICON_NEXT{"<" DEF_ICON_NEXT ">"};
    static constexpr const char* NAME_ICON_SEEKB{DEF_ICON_SEEKB};
    static constexpr const char* TAG_ICON_SEEKB{"<" DEF_ICON_SEEKB ">"};
    static constexpr const char* NAME_ICON_SEEKF{DEF_ICON_SEEKF};
    static constexpr const char* TAG_ICON_SEEKF{"<" DEF_ICON_SEEKF ">"};

    static constexpr const char* FORMAT_OFFLINE{"format-offline"};
    static constexpr const char* NAME_LABEL_OFFLINE{DEF_LABEL_OFFLINE};
    static constexpr const char* TAG_LABEL_OFFLINE{"<" DEF_LABEL_OFFLINE ">"};

    unique_ptr<mpdconnection> m_mpd;

    /*
     * Stores the mpdstatus instance for the current connection
     * m_status is not initialized if mpd is not connect, you always have to
     * make sure that m_status is not NULL before dereferencing it
     */
    unique_ptr<mpdstatus> m_status;

    string m_host{env_util::get("MPD_HOST", "127.0.0.1")};
    string m_pass;
    unsigned int m_port{6600U};

    chrono::steady_clock::time_point m_lastsync{};
    float m_synctime{1.0f};

    int m_quick_attempts{0};

    // This flag is used to let thru a broadcast once every time
    // the connection state changes
    connection_state m_statebroadcasted{connection_state::NONE};

    progressbar_t m_bar_progress;
    iconset_t m_icons;
    label_t m_label_song;
    label_t m_label_time;
    label_t m_label_offline;

    rgba m_toggle_on_color;
    rgba m_toggle_off_color;
  };
}  // namespace modules

POLYBAR_NS_END
