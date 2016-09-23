#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "modules/base.hpp"
#include "interfaces/net.hpp"
#include "services/logger.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/ramp.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/format.hpp"

using namespace boost::posix_time;
namespace modules
{
  DefineModule(NetworkModule, TimerModule)
  {
    static constexpr auto FORMAT_CONNECTED = "format-connected";
    static constexpr auto FORMAT_PACKETLOSS = "format-packetloss";
    static constexpr auto FORMAT_DISCONNECTED = "format-disconnected";

    static constexpr auto TAG_RAMP_SIGNAL = "<ramp-signal>";
    static constexpr auto TAG_LABEL_CONNECTED = "<label-connected>";
    static constexpr auto TAG_LABEL_DISCONNECTED = "<label-disconnected>";
    static constexpr auto TAG_LABEL_PACKETLOSS = "<label-packetloss>";
    static constexpr auto TAG_ANIMATION_PACKETLOSS = "<animation-packetloss>";

    std::unique_ptr<net::WiredNetwork> wired_network;
    std::unique_ptr<net::WirelessNetwork> wireless_network;

    std::unique_ptr<drawtypes::Ramp> ramp_signal;
    std::unique_ptr<drawtypes::Animation> animation_packetloss;
    std::unique_ptr<drawtypes::Label> label_connected;
    std::unique_ptr<drawtypes::Label> label_connected_tokenized;
    std::unique_ptr<drawtypes::Label> label_disconnected;
    std::unique_ptr<drawtypes::Label> label_packetloss;
    std::unique_ptr<drawtypes::Label> label_packetloss_tokenized;

    std::shared_ptr<Logger> logger;

    std::string interface;

    concurrency::Atomic<bool> connected;
    concurrency::Atomic<bool> conseq_packetloss;
    concurrency::Atomic<int> signal_quality;
    concurrency::Atomic<long long> last_rx_bytes;
    concurrency::Atomic<long long> last_tx_bytes;

    // Speeds are in bytes per seconds
    concurrency::Atomic<float> current_rx_speed;
    concurrency::Atomic<float> current_tx_speed;

    ptime last_update;

    int ping_nth_update;
    int counter = -1; // Set to -1 to ignore the first run

    void subthread_routine();

    public:
      explicit NetworkModule(std::string name);

      void start();
      bool update();

      std::string get_format();

      bool build(Builder *builder, std::string tag);

    protected:
      std::string get_upspeed();
      std::string get_downspeed();
      void calculate_netspeeds();
      // Speed is in bytes per second
      std::string format_netspeed(float speed);

  };
}
