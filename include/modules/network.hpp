#pragma once

#include "adapters/net.hpp"
#include "components/config.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class connection_state { NONE = 0, CONNECTED, DISCONNECTED, PACKETLOSS };

  class network_module : public timer_module<network_module> {
   public:
    using timer_module::timer_module;

    void setup();
    void teardown();
    bool update();
    string get_format() const;
    bool build(builder* builder, string tag) const;

   protected:
    void subthread_routine();

   private:
    static constexpr auto FORMAT_CONNECTED = "format-connected";
    static constexpr auto FORMAT_PACKETLOSS = "format-packetloss";
    static constexpr auto FORMAT_DISCONNECTED = "format-disconnected";
    static constexpr auto TAG_RAMP_SIGNAL = "<ramp-signal>";
    static constexpr auto TAG_RAMP_QUALITY = "<ramp-quality>";
    static constexpr auto TAG_LABEL_CONNECTED = "<label-connected>";
    static constexpr auto TAG_LABEL_DISCONNECTED = "<label-disconnected>";
    static constexpr auto TAG_LABEL_PACKETLOSS = "<label-packetloss>";
    static constexpr auto TAG_ANIMATION_PACKETLOSS = "<animation-packetloss>";

    net::wired_t m_wired;
    net::wireless_t m_wireless;

    ramp_t m_ramp_signal;
    ramp_t m_ramp_quality;
    animation_t m_animation_packetloss;
    map<connection_state, label_t> m_label;

    stateflag m_connected{false};
    stateflag m_packetloss{false};

    int m_signal = 0;
    int m_quality = 0;
    int m_counter = -1;  // -1 to ignore the first run

    string m_interface;
    int m_ping_nth_update = 0;
    int m_udspeed_minwidth = 3;
  };
}

LEMONBUDDY_NS_END
