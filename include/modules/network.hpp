#pragma once

#include "adapters/net.hpp"
#include "components/config.hpp"
#include "modules/meta/timer_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

namespace modules {
  enum class connection_state { NONE = 0, CONNECTED, DISCONNECTED, PACKETLOSS };

  class network_module : public timer_module<network_module>,
                         public input_handler {
   public:
    explicit network_module(const bar_settings&, string);

    void teardown();
    bool update();
    string get_format() const;
    bool build(builder* builder, const string& tag) const;
    string get_output();

   protected:
    void subthread_routine();
    bool input(string&& cmd);

   private:
    static constexpr auto FORMAT_CONNECTED = "format-connected";
    static constexpr auto FORMAT_PACKETLOSS = "format-packetloss";
    static constexpr auto FORMAT_DISCONNECTED = "format-disconnected";
    static constexpr auto TAG_RAMP_SIGNAL = "<ramp-signal>";
    static constexpr auto TAG_RAMP_QUALITY = "<ramp-quality>";
    static constexpr auto TAG_LABEL_CONNECTED = "<label-connected>";
    static constexpr auto TAG_LABEL_ALT_CONNECTED = "<label-alt-connected>";
    static constexpr auto TAG_LABEL_DISCONNECTED = "<label-disconnected>";
    static constexpr auto TAG_LABEL_PACKETLOSS = "<label-packetloss>";
    static constexpr auto TAG_LABEL_ALT_PACKETLOSS = "<label-alt-packetloss>";
    static constexpr auto TAG_ANIMATION_PACKETLOSS = "<animation-packetloss>";
    static constexpr auto EVENT_TOGGLE = "networktoggle";

    net::wired_t m_wired;
    net::wireless_t m_wireless;

    ramp_t m_ramp_signal;
    ramp_t m_ramp_quality;
    animation_t m_animation_packetloss;
    map<connection_state, label_t> m_label;
    map<connection_state, label_t> m_label_alt;

    atomic<bool> m_connected{false};
    atomic<bool> m_packetloss{false};
    atomic<bool> m_toggleable{false};
    atomic<bool> m_toggled{false};

    int m_signal{0};
    int m_quality{0};
    int m_counter{-1};  // -1 to ignore the first run

    string m_interface;
    int m_ping_nth_update{0};
    int m_udspeed_minwidth{0};
    bool m_accumulate{false};
  };
}

POLYBAR_NS_END
