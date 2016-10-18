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

    void setup() {
      // Load configuration values
      REQ_CONFIG_VALUE(name(), m_interface, "interface");
      GET_CONFIG_VALUE(name(), m_ping_nth_update, "ping-interval");
      GET_CONFIG_VALUE(name(), m_udspeed_minwidth, "udspeed-minwidth");

      m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 1));

      // Add formats
      m_formatter->add(FORMAT_CONNECTED, TAG_LABEL_CONNECTED,
          {TAG_RAMP_SIGNAL, TAG_RAMP_QUALITY, TAG_LABEL_CONNECTED});
      m_formatter->add(FORMAT_DISCONNECTED, TAG_LABEL_DISCONNECTED, {TAG_LABEL_DISCONNECTED});

      // Create elements for format-connected
      if (m_formatter->has(TAG_RAMP_SIGNAL, FORMAT_CONNECTED))
        m_ramp_signal = get_config_ramp(m_conf, name(), TAG_RAMP_SIGNAL);
      if (m_formatter->has(TAG_RAMP_QUALITY, FORMAT_CONNECTED))
        m_ramp_quality = get_config_ramp(m_conf, name(), TAG_RAMP_QUALITY);
      if (m_formatter->has(TAG_LABEL_CONNECTED, FORMAT_CONNECTED)) {
        m_label[connection_state::CONNECTED] =
            get_optional_config_label(m_conf, name(), TAG_LABEL_CONNECTED, "%ifname% %local_ip%");
        m_tokenized[connection_state::CONNECTED] = m_label[connection_state::CONNECTED]->clone();
      }

      // Create elements for format-disconnected
      if (m_formatter->has(TAG_LABEL_DISCONNECTED, FORMAT_DISCONNECTED)) {
        m_label[connection_state::DISCONNECTED] =
            get_optional_config_label(m_conf, name(), TAG_LABEL_DISCONNECTED, "");
        m_label[connection_state::DISCONNECTED]->replace_token("%ifname%", m_interface);
      }

      // Create elements for format-packetloss if we are told to test connectivity
      if (m_ping_nth_update > 0) {
        m_formatter->add(FORMAT_PACKETLOSS, TAG_LABEL_CONNECTED,
            {TAG_ANIMATION_PACKETLOSS, TAG_LABEL_PACKETLOSS, TAG_LABEL_CONNECTED});

        if (m_formatter->has(TAG_LABEL_PACKETLOSS, FORMAT_PACKETLOSS)) {
          m_label[connection_state::PACKETLOSS] =
              get_optional_config_label(m_conf, name(), TAG_LABEL_PACKETLOSS, "");
          m_tokenized[connection_state::PACKETLOSS] =
              m_label[connection_state::PACKETLOSS]->clone();
        }
        if (m_formatter->has(TAG_ANIMATION_PACKETLOSS, FORMAT_PACKETLOSS))
          m_animation_packetloss = get_config_animation(m_conf, name(), TAG_ANIMATION_PACKETLOSS);
      }

      // Get an intstance of the network interface
      if (net::is_wireless_interface(m_interface))
        m_wireless = net::wireless_t{new net::wireless_t::element_type(m_interface)};
      else
        m_wired = net::wired_t{new net::wired_t::element_type(m_interface)};

      // We only need to start the subthread if the packetloss animation is used
      if (m_animation_packetloss)
        m_threads.emplace_back(thread(&network_module::subthread_routine, this));
    }

    bool update() {
      net::network* network = m_wireless ? dynamic_cast<net::network*>(m_wireless.get())
                                         : dynamic_cast<net::network*>(m_wired.get());

      if (!network->query()) {
        m_log.warn("%s: Failed to query interface '%s'", name(), m_interface);
        return false;
      }

      try {
        if (m_wireless) {
          m_signal = m_wireless->signal();
          m_quality = m_wireless->quality();
        }
      } catch (const net::network_error& err) {
        m_log.warn("%s: Error getting interface data (%s)", name(), err.what());
      }

      m_connected = network->connected();

      // Ignore the first run
      if (m_counter == -1) {
        m_counter = 0;
      } else if (m_ping_nth_update > 0 && m_connected && (++m_counter % m_ping_nth_update) == 0) {
        m_packetloss = !network->ping();
        m_counter = 0;
      }

      auto upspeed = network->upspeed(m_udspeed_minwidth);
      auto downspeed = network->downspeed(m_udspeed_minwidth);

      // Update label contents
      const auto replace_tokens = [&](label_t& label) {
        label->replace_token("%ifname%", m_interface);
        label->replace_token("%local_ip%", network->ip());
        label->replace_token("%upspeed%", upspeed);
        label->replace_token("%downspeed%", downspeed);

        if (m_wired) {
          label->replace_token("%linkspeed%", m_wired->linkspeed());
        } else if (m_wireless) {
          label->replace_token("%essid%", m_wireless->essid());
          label->replace_token("%signal%", to_string(m_signal) + "%");
          label->replace_token("%quality%", to_string(m_quality) + "%");
        }
      };

      if (m_label[connection_state::CONNECTED]) {
        m_tokenized[connection_state::CONNECTED]->m_text =
            m_label[connection_state::CONNECTED]->m_text;
        replace_tokens(m_tokenized[connection_state::CONNECTED]);
      }

      if (m_label[connection_state::PACKETLOSS]) {
        m_tokenized[connection_state::PACKETLOSS]->m_text =
            m_label[connection_state::PACKETLOSS]->m_text;
        replace_tokens(m_tokenized[connection_state::PACKETLOSS]);
      }

      return true;
    }

    string get_format() {
      if (!m_connected)
        return FORMAT_DISCONNECTED;
      else if (m_packetloss && m_ping_nth_update > 0)
        return FORMAT_PACKETLOSS;
      else
        return FORMAT_CONNECTED;
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_LABEL_CONNECTED)
        builder->node(m_tokenized[connection_state::CONNECTED]);
      else if (tag == TAG_LABEL_DISCONNECTED)
        builder->node(m_label[connection_state::DISCONNECTED]);
      else if (tag == TAG_LABEL_PACKETLOSS)
        builder->node(m_tokenized[connection_state::PACKETLOSS]);
      else if (tag == TAG_ANIMATION_PACKETLOSS)
        builder->node(m_animation_packetloss->get());
      else if (tag == TAG_RAMP_SIGNAL)
        builder->node(m_ramp_signal->get_by_percentage(m_signal));
      else if (tag == TAG_RAMP_QUALITY)
        builder->node(m_ramp_quality->get_by_percentage(m_quality));
      else
        return false;
      return true;
    }

   protected:
    void subthread_routine() {
      const chrono::milliseconds framerate{m_animation_packetloss->framerate()};
      const auto dur = chrono::duration<double>(framerate);

      while (enabled()) {
        if (m_connected && m_packetloss)
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
    map<connection_state, label_t> m_tokenized;

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
