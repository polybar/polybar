#pragma once

#include <chrono>

#include "utils/env.hpp"
#include "adapters/mpd.hpp"
#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

using namespace mpd;

namespace modules {
  class mpd_module : public event_module<mpd_module>, public input_handler {
   public:
    explicit mpd_module(const bar_settings&, string);

    void teardown();
    inline bool connected() const;
    void idle();
    bool has_event();
    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool input(string&& cmd);

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

    static constexpr const char* EVENT_PLAY{"mpdplay"};
    static constexpr const char* EVENT_PAUSE{"mpdpause"};
    static constexpr const char* EVENT_STOP{"mpdstop"};
    static constexpr const char* EVENT_PREV{"mpdprev"};
    static constexpr const char* EVENT_NEXT{"mpdnext"};
    static constexpr const char* EVENT_REPEAT{"mpdrepeat"};
    static constexpr const char* EVENT_SINGLE{"mpdsingle"};
    static constexpr const char* EVENT_RANDOM{"mpdrandom"};
    static constexpr const char* EVENT_CONSUME{"mpdconsume"};
    static constexpr const char* EVENT_SEEK{"mpdseek"};

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

    chrono::system_clock::time_point m_lastsync{};
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

    string m_toggle_on_color;
    string m_toggle_off_color;
  };
}

POLYBAR_NS_END
