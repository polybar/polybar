#pragma once

#include <chrono>

#include "adapters/mpris.hpp"
#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

using namespace mpris;

namespace modules {
  class mpris_module : public event_module<mpris_module>, public input_handler {
   public:
    explicit mpris_module(const bar_settings&, string);

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
    static constexpr const char* TAG_BAR_PROGRESS{"<bar-progress>"};
    static constexpr const char* TAG_TOGGLE{"<toggle>"};
    static constexpr const char* TAG_TOGGLE_STOP{"<toggle-stop>"};
    static constexpr const char* TAG_LABEL_SONG{"<label-song>"};
    static constexpr const char* TAG_LABEL_TIME{"<label-time>"};
    static constexpr const char* TAG_ICON_RANDOM{"<icon-random>"};
    static constexpr const char* TAG_ICON_REPEAT{"<icon-repeat>"};
    static constexpr const char* TAG_ICON_REPEAT_ONE{"<icon-repeatone>"};
    static constexpr const char* TAG_ICON_PREV{"<icon-prev>"};
    static constexpr const char* TAG_ICON_STOP{"<icon-stop>"};
    static constexpr const char* TAG_ICON_PLAY{"<icon-play>"};
    static constexpr const char* TAG_ICON_PAUSE{"<icon-pause>"};
    static constexpr const char* TAG_ICON_NEXT{"<icon-next>"};
    static constexpr const char* TAG_ICON_SEEKB{"<icon-seekb>"};
    static constexpr const char* TAG_ICON_SEEKF{"<icon-seekf>"};

    static constexpr const char* FORMAT_OFFLINE{"format-offline"};
    static constexpr const char* TAG_LABEL_OFFLINE{"<label-offline>"};

    static constexpr const char* EVENT_PLAY{"mprisplay"};
    static constexpr const char* EVENT_PAUSE{"mprispause"};
    static constexpr const char* EVENT_STOP{"mprisstop"};
    static constexpr const char* EVENT_PREV{"mprisprev"};
    static constexpr const char* EVENT_NEXT{"mprisnext"};
    static constexpr const char* EVENT_REPEAT{"mprisrepeat"};
    static constexpr const char* EVENT_REPEAT_ONE{"mprisrepeatone"};
    static constexpr const char* EVENT_RANDOM{"mprisrandom"};
    static constexpr const char* EVENT_SEEK{"mprisseek"};

    unique_ptr<mprisconnection> m_connection;

    mprissong m_song;
    unique_ptr<mprisstatus> m_status;
    //unique_ptr<mprissong> m_song;

    string m_player;

    chrono::system_clock::time_point m_lastsync{};
    float m_synctime{1.0f};

    int m_quick_attempts{0};

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
