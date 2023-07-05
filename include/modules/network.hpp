#pragma once

#include "adapters/net.hpp"
#include "components/config.hpp"
#include "modules/meta/timer_module.hpp"
#include "modules/meta/types.hpp"

POLYBAR_NS

namespace modules {
  enum class connection_state { NONE = 0, CONNECTED, DISCONNECTED, PACKETLOSS };

  class network_module : public timer_module<network_module> {
   public:
    explicit network_module(const bar_settings&, string, const config&);

    void teardown();
    bool update();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = NETWORK_TYPE;

   protected:
    void subthread_routine();

   private:
    static constexpr auto FORMAT_CONNECTED = "format-connected";
    static constexpr auto FORMAT_PACKETLOSS = "format-packetloss";
    static constexpr auto FORMAT_DISCONNECTED = "format-disconnected";
#define DEF_RAMP_SIGNAL "<ramp-signal"
#define DEF_RAMP_QUALITY "<ramp-quality"
#define DEF_LABEL_CONNECTED "<label-connected"
#define DEF_LABEL_DISCONNECTED "<label-disconnected"
#define DEF_LABEL_PACKETLOSS "<label-packetloss"
#define DEF_ANIMATION_PACKETLOSS "<animation-packetloss"
    static constexpr auto NAME_RAMP_SIGNAL = DEF_RAMP_SIGNAL;
    static constexpr auto NAME_RAMP_QUALITY = DEF_RAMP_QUALITY;
    static constexpr auto NAME_LABEL_CONNECTED = DEF_LABEL_CONNECTED;
    static constexpr auto NAME_LABEL_DISCONNECTED = DEF_LABEL_DISCONNECTED;
    static constexpr auto NAME_LABEL_PACKETLOSS = DEF_LABEL_PACKETLOSS;
    static constexpr auto NAME_ANIMATION_PACKETLOSS = DEF_ANIMATION_PACKETLOSS;

    static constexpr auto TAG_RAMP_SIGNAL = "<" DEF_RAMP_SIGNAL ">";
    static constexpr auto TAG_RAMP_QUALITY = "<" DEF_RAMP_QUALITY ">";
    static constexpr auto TAG_LABEL_CONNECTED = "<" DEF_LABEL_CONNECTED ">";
    static constexpr auto TAG_LABEL_DISCONNECTED = "<" DEF_LABEL_DISCONNECTED ">";
    static constexpr auto TAG_LABEL_PACKETLOSS = "<" DEF_LABEL_PACKETLOSS ">";
    static constexpr auto TAG_ANIMATION_PACKETLOSS = "<" DEF_ANIMATION_PACKETLOSS ">";
#undef DEF_RAMP_SIGNAL
#undef DEF_RAMP_QUALITY
#undef DEF_LABEL_CONNECTED
#undef DEF_LABEL_DISCONNECTED
#undef DEF_LABEL_PACKETLOSS
#undef DEF_ANIMATION_PACKETLOSS

    net::wired_t m_wired;
    net::wireless_t m_wireless;

    ramp_t m_ramp_signal;
    ramp_t m_ramp_quality;
    animation_t m_animation_packetloss;
    map<connection_state, label_t> m_label;

    atomic<bool> m_connected{false};
    atomic<bool> m_packetloss{false};

    int m_signal{0};
    int m_quality{0};
    int m_counter{-1};  // -1 to ignore the first run

    string m_interface;
    int m_ping_nth_update{0};
    int m_udspeed_minwidth{0};
    bool m_accumulate{false};
    bool m_unknown_up{false};
    string m_udspeed_unit{"B/s"};
  };
}  // namespace modules

POLYBAR_NS_END
