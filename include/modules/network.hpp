#pragma once

#include "adapters/net.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

#define DEFAULT_FORMAT_CONNECTED TAG_LABEL_CONNECTED
#define DEFAULT_FORMAT_DISCONNECTED TAG_LABEL_DISCONNECTED
#define DEFAULT_FORMAT_PACKETLOSS TAG_LABEL_CONNECTED

#define DEFAULT_LABEL_CONNECTED "%ifname% %local_ip%"
#define DEFAULT_LABEL_DISCONNECTED ""
#define DEFAULT_LABEL_PACKETLOSS ""

LEMONBUDDY_NS

namespace modules {

  class network_module : public timer_module<network_module> {
   public:
    using timer_module::timer_module;

    void setup() {
      // Load configuration values
      m_interface = m_conf.get<string>(name(), "interface");
      m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));
      m_ping_nth_update = m_conf.get<int>(name(), "ping-interval", 0);
      m_udspeed_minwidth = m_conf.get<int>(name(), "udspeed-minwidth", m_udspeed_minwidth);

      // Add formats
      m_formatter->add(
          FORMAT_CONNECTED, DEFAULT_FORMAT_CONNECTED, {TAG_RAMP_SIGNAL, TAG_LABEL_CONNECTED});
      m_formatter->add(FORMAT_DISCONNECTED, DEFAULT_FORMAT_DISCONNECTED, {TAG_LABEL_DISCONNECTED});

      // Create elements for format-connected
      if (m_formatter->has(TAG_RAMP_SIGNAL, FORMAT_CONNECTED))
        m_ramp_signal = get_config_ramp(m_conf, name(), TAG_RAMP_SIGNAL);
      if (m_formatter->has(TAG_LABEL_CONNECTED, FORMAT_CONNECTED)) {
        m_label_connected =
            get_optional_config_label(m_conf, name(), TAG_LABEL_CONNECTED, DEFAULT_LABEL_CONNECTED);
        m_label_connected_tokenized = m_label_connected->clone();
      }

      // Create elements for format-disconnected
      if (m_formatter->has(TAG_LABEL_DISCONNECTED, FORMAT_DISCONNECTED)) {
        m_label_disconnected = get_optional_config_label(
            m_conf, name(), TAG_LABEL_DISCONNECTED, DEFAULT_LABEL_DISCONNECTED);
        m_label_disconnected->replace_token("%ifname%", m_interface);
      }

      // Create elements for format-packetloss if we are told to test connectivity
      if (m_ping_nth_update > 0) {
        m_formatter->add(FORMAT_PACKETLOSS, DEFAULT_FORMAT_PACKETLOSS,
            {TAG_ANIMATION_PACKETLOSS, TAG_LABEL_PACKETLOSS, TAG_LABEL_CONNECTED});

        if (m_formatter->has(TAG_LABEL_PACKETLOSS, FORMAT_PACKETLOSS)) {
          m_label_packetloss = get_optional_config_label(
              m_conf, name(), TAG_LABEL_PACKETLOSS, DEFAULT_LABEL_PACKETLOSS);
          m_label_packetloss_tokenized = m_label_packetloss->clone();
        }
        if (m_formatter->has(TAG_ANIMATION_PACKETLOSS, FORMAT_PACKETLOSS))
          m_animation_packetloss = get_config_animation(m_conf, name(), TAG_ANIMATION_PACKETLOSS);
      }

      // Get an intstance of the network interface
      try {
        if (net::is_wireless_interface(m_interface)) {
          m_wireless_network = make_unique<net::wireless_network>(m_interface);
        } else {
          m_wired_network = make_unique<net::wired_network>(m_interface);
        }
      } catch (net::network_error& e) {
        m_log.err("%s: %s", name(), e.what());
        std::exit(EXIT_FAILURE);
      }
    }

    void start() {
      timer_module::start();

      // We only need to start the subthread if the packetloss animation is used
      if (m_animation_packetloss)
        m_threads.emplace_back(thread(&network_module::subthread_routine, this));
    }
    bool update() {
      string ip, essid, linkspeed;
      int signal_quality = 0;

      net::network* network = nullptr;

      // Process data for wireless network interfaces
      if (m_wireless_network) {
        network = m_wireless_network.get();

        try {
          essid = m_wireless_network->essid();
          signal_quality = m_wireless_network->signal_quality();
        } catch (net::wireless_network_error& e) {
          m_log.trace("%s: %s", name(), e.what());
        }

        m_signal_quality = signal_quality;

        // Process data for wired network interfaces
      } else if (m_wired_network) {
        network = m_wired_network.get();
        linkspeed = m_wired_network->link_speed();
      }

      if (network != nullptr) {
        try {
          ip = network->ip();
        } catch (net::network_error& e) {
          m_log.trace("%s: %s", name(), e.what());
        }

        m_connected = network->connected();

        // Ignore the first run
        if (m_counter == -1) {
          m_counter = 0;
        } else if (m_ping_nth_update > 0 && m_connected && (++m_counter % m_ping_nth_update) == 0) {
          m_conseq_packetloss = !network->test();
          m_counter = 0;
        }
      }

      // Update label contents
      if (m_label_connected || m_label_packetloss) {
        auto replace_tokens = [&](label_t label) {
          label->replace_token("%ifname%", m_interface);
          label->replace_token("%local_ip%", !ip.empty() ? ip : "x.x.x.x");

          if (m_wired_network) {
            label->replace_token("%linkspeed%", linkspeed);
          } else if (m_wireless_network) {
            label->replace_token("%essid%", !essid.empty() ? essid : "UNKNOWN");
            label->replace_token("%signal%", to_string(signal_quality) + "%");
          }

          auto upspeed = network->upspeed(m_udspeed_minwidth);
          auto downspeed = network->downspeed(m_udspeed_minwidth);

          label->replace_token("%upspeed%", upspeed);
          label->replace_token("%downspeed%", downspeed);
        };

        if (m_label_connected) {
          m_label_connected_tokenized->m_text = m_label_connected->m_text;
          replace_tokens(m_label_connected_tokenized);
        }

        if (m_label_packetloss) {
          m_label_packetloss_tokenized->m_text = m_label_packetloss->m_text;
          replace_tokens(m_label_packetloss_tokenized);
        }
      }

      return true;
    }

    string get_format() {
      if (!m_connected)
        return FORMAT_DISCONNECTED;
      else if (m_conseq_packetloss && m_ping_nth_update > 0)
        return FORMAT_PACKETLOSS;
      else
        return FORMAT_CONNECTED;
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_LABEL_CONNECTED)
        builder->node(m_label_connected_tokenized);
      else if (tag == TAG_LABEL_DISCONNECTED)
        builder->node(m_label_disconnected);
      else if (tag == TAG_LABEL_PACKETLOSS)
        builder->node(m_label_packetloss_tokenized);
      else if (tag == TAG_ANIMATION_PACKETLOSS)
        builder->node(m_animation_packetloss->get());
      else if (tag == TAG_RAMP_SIGNAL)
        builder->node(m_ramp_signal->get_by_percentage(m_signal_quality));
      else
        return false;
      return true;
    }

   protected:
    void subthread_routine() {
      this_thread::yield();

      const auto dur =
          chrono::duration<double>(float(m_animation_packetloss->framerate()) / 1000.0f);

      while (enabled()) {
        if (m_connected && m_conseq_packetloss)
          broadcast();

        sleep(dur);
      }

      m_log.trace("%s: Reached end of network subthread", name());
    }

   private:
    static constexpr auto FORMAT_CONNECTED = "format-connected";
    static constexpr auto FORMAT_PACKETLOSS = "format-packetloss";
    static constexpr auto FORMAT_DISCONNECTED = "format-disconnected";

    static constexpr auto TAG_RAMP_SIGNAL = "<ramp-signal>";
    static constexpr auto TAG_LABEL_CONNECTED = "<label-connected>";
    static constexpr auto TAG_LABEL_DISCONNECTED = "<label-disconnected>";
    static constexpr auto TAG_LABEL_PACKETLOSS = "<label-packetloss>";
    static constexpr auto TAG_ANIMATION_PACKETLOSS = "<animation-packetloss>";

    unique_ptr<net::wired_network> m_wired_network;
    unique_ptr<net::wireless_network> m_wireless_network;

    ramp_t m_ramp_signal;
    animation_t m_animation_packetloss;
    label_t m_label_connected;
    label_t m_label_connected_tokenized;
    label_t m_label_disconnected;
    label_t m_label_packetloss;
    label_t m_label_packetloss_tokenized;

    stateflag m_connected{false};
    stateflag m_conseq_packetloss{false};

    string m_interface;
    int m_signal_quality;
    int m_ping_nth_update;
    int m_counter = -1;  // -1 to ignore the first run
    int m_udspeed_minwidth = 3;
  };
}

LEMONBUDDY_NS_END
