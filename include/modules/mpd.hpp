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
    static constexpr const char* FORMAT_ONLINE{"format-online"};
    static constexpr const char* FORMAT_PLAYING{"format-playing"};
    static constexpr const char* FORMAT_PAUSED{"format-paused"};
    static constexpr const char* FORMAT_STOPPED{"format-stopped"};
    static constexpr const char* TAG_BAR_PROGRESS{"<bar-progress>"};
    static constexpr const char* TAG_TOGGLE{"<toggle>"};
    static constexpr const char* TAG_TOGGLE_STOP{"<toggle-stop>"};
    static constexpr const char* TAG_LABEL_SONG{"<label-song>"};
    static constexpr const char* TAG_LABEL_TIME{"<label-time>"};
    static constexpr const char* TAG_ICON_RANDOM{"<icon-random>"};
    static constexpr const char* TAG_ICON_REPEAT{"<icon-repeat>"};
    /*
     * Deprecated
     */
    static constexpr const char* TAG_ICON_REPEAT_ONE{"<icon-repeatone>"};
    /*
     * Replaces icon-repeatone
     *
     * repeatone is misleading, since it doesn't actually affect the repeating behaviour
     */
    static constexpr const char* TAG_ICON_SINGLE{"<icon-single>"};
    static constexpr const char* TAG_ICON_CONSUME{"<icon-consume>"};
    static constexpr const char* TAG_ICON_PREV{"<icon-prev>"};
    static constexpr const char* TAG_ICON_STOP{"<icon-stop>"};
    static constexpr const char* TAG_ICON_PLAY{"<icon-play>"};
    static constexpr const char* TAG_ICON_PAUSE{"<icon-pause>"};
    static constexpr const char* TAG_ICON_NEXT{"<icon-next>"};
    static constexpr const char* TAG_ICON_SEEKB{"<icon-seekb>"};
    static constexpr const char* TAG_ICON_SEEKF{"<icon-seekf>"};

    static constexpr const char* FORMAT_OFFLINE{"format-offline"};
    static constexpr const char* TAG_LABEL_OFFLINE{"<label-offline>"};

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
