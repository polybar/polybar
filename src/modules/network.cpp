#include <thread>

#include "lemonbuddy.hpp"
#include "modules/network.hpp"
#include "services/logger.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/proc.hpp"

using namespace modules;

// TODO: Add up-/download speed (check how ifconfig read the bytes)

NetworkModule::NetworkModule(const std::string& name_)
  : TimerModule(name_, 1s), connected(false), conseq_packetloss(false)
{
  static const auto DEFAULT_FORMAT_CONNECTED = TAG_LABEL_CONNECTED;
  static const auto DEFAULT_FORMAT_DISCONNECTED = TAG_LABEL_DISCONNECTED;
  static const auto DEFAULT_FORMAT_PACKETLOSS = TAG_LABEL_CONNECTED;

  static const auto DEFAULT_LABEL_CONNECTED = "%ifname% %local_ip%";
  static const auto DEFAULT_LABEL_DISCONNECTED = "";
  static const auto DEFAULT_LABEL_PACKETLOSS = "";

  // Load configuration values
  this->interface = config::get<std::string>(name(), "interface");
  this->interval = std::chrono::duration<double>(config::get<float>(name(), "interval", 1));
  this->ping_nth_update = config::get<int>(name(), "ping_interval", 0);

  // Add formats
  this->formatter->add(FORMAT_CONNECTED, DEFAULT_FORMAT_CONNECTED, { TAG_RAMP_SIGNAL, TAG_LABEL_CONNECTED });
  this->formatter->add(FORMAT_DISCONNECTED, DEFAULT_FORMAT_DISCONNECTED, { TAG_LABEL_DISCONNECTED });

  // Create elements for format-connected
  if (this->formatter->has(TAG_RAMP_SIGNAL, FORMAT_CONNECTED))
    this->ramp_signal = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP_SIGNAL));
  if (this->formatter->has(TAG_LABEL_CONNECTED, FORMAT_CONNECTED))
    this->label_connected = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_CONNECTED), DEFAULT_LABEL_CONNECTED);

  // Create elements for format-disconnected
  if (this->formatter->has(TAG_LABEL_DISCONNECTED, FORMAT_DISCONNECTED)) {
    this->label_disconnected = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_DISCONNECTED), DEFAULT_LABEL_DISCONNECTED);
    this->label_disconnected->replace_token("%ifname%", this->interface);
  }

  // Create elements for format-packetloss if we are told to test connectivity
  if (this->ping_nth_update > 0) {
    this->formatter->add(FORMAT_PACKETLOSS, DEFAULT_FORMAT_PACKETLOSS, { TAG_ANIMATION_PACKETLOSS, TAG_LABEL_PACKETLOSS, TAG_LABEL_CONNECTED });

    if (this->formatter->has(TAG_LABEL_PACKETLOSS, FORMAT_PACKETLOSS))
      this->label_packetloss = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_PACKETLOSS), DEFAULT_LABEL_PACKETLOSS);
    if (this->formatter->has(TAG_ANIMATION_PACKETLOSS, FORMAT_PACKETLOSS))
      this->animation_packetloss = drawtypes::get_config_animation(name(), get_tag_name(TAG_ANIMATION_PACKETLOSS));
  }

  // Get an intstance of the network interface
  try {
    if (net::is_wireless_interface(this->interface)) {
      this->wireless_network = std::make_unique<net::WirelessNetwork>(this->interface);
    } else {
      this->wired_network = std::make_unique<net::WiredNetwork>(this->interface);
    }
  } catch (net::NetworkException &e) {
    get_logger()->fatal(e.what());
  }
}

void NetworkModule::start()
{
  this->TimerModule::start();

  // We only need to start the subthread if the packetloss animation is used
  if (this->animation_packetloss)
    this->threads.emplace_back(std::thread(&NetworkModule::subthread_routine, this));
}

void NetworkModule::subthread_routine()
{
  std::this_thread::yield();

  const auto dur = std::chrono::duration<double>(
    float(this->animation_packetloss->get_framerate()) / 1000.0f);

  while (this->enabled()) {
    if (this->connected && this->conseq_packetloss)
      this->broadcast();

    std::this_thread::sleep_for(dur);
  }

  log_debug("Reached end of network subthread");
}

bool NetworkModule::update()
{
  std::string ip, essid, linkspeed;
  int signal_quality = 0;

  net::Network *network = nullptr;

  // Process data for wireless network interfaces
  if (this->wireless_network) {
    network = this->wireless_network.get();

    try {
      essid = this->wireless_network->get_essid();
      signal_quality = this->wireless_network->get_signal_quality();
    } catch (net::WirelessNetworkException &e) {
      get_logger()->debug(e.what());
    }

    this->signal_quality = signal_quality;

  // Process data for wired network interfaces
  } else if (this->wired_network) {
    network = this->wired_network.get();
    linkspeed = this->wired_network->get_link_speed();
  }

  if (network != nullptr) {
    try {
      ip = network->get_ip();
    } catch (net::NetworkException &e) {
      get_logger()->debug(e.what());
    }

    this->connected = network->connected();

    // Ignore the first run
    if (this->counter == -1) {
      this->counter = 0;
    } else if (this->ping_nth_update > 0 && this->connected && (++this->counter % this->ping_nth_update) == 0) {
      this->conseq_packetloss = !network->test();
      this->counter = 0;
    }
  }

  // Update label contents
  if (this->label_connected || this->label_packetloss) {
    auto replace_tokens = [&](std::unique_ptr<drawtypes::Label> &label){
      label->replace_token("%ifname%", this->interface);
      label->replace_token("%local_ip%", ip);

      if (this->wired_network) {
        label->replace_token("%linkspeed%", linkspeed);
      } else if (this->wireless_network) {
        label->replace_token("%essid%", !essid.empty() ? essid : "No network");
        label->replace_token("%signal%", std::to_string(signal_quality)+"%");
      }
    };

    if (this->label_connected) {
      if (!this->label_connected_tokenized)
        this->label_connected_tokenized = this->label_connected->clone();
      this->label_connected_tokenized->text = this->label_connected->text;

      replace_tokens(this->label_connected_tokenized);
    }

    if (this->label_packetloss) {
      if (!this->label_packetloss_tokenized)
        this->label_packetloss_tokenized = this->label_packetloss->clone();
      this->label_packetloss_tokenized->text = this->label_packetloss->text;

      replace_tokens(this->label_packetloss_tokenized);
    }
  }

  return true;
}

std::string NetworkModule::get_format()
{
  if (!this->connected)
    return FORMAT_DISCONNECTED;
  else if (this->conseq_packetloss && this->ping_nth_update > 0)
    return FORMAT_PACKETLOSS;
  else
    return FORMAT_CONNECTED;
}

bool NetworkModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_LABEL_CONNECTED)
    builder->node(this->label_connected_tokenized);
  else if (tag == TAG_LABEL_DISCONNECTED)
    builder->node(this->label_disconnected);
  else if (tag == TAG_LABEL_PACKETLOSS)
    builder->node(this->label_packetloss_tokenized);
  else if (tag == TAG_ANIMATION_PACKETLOSS)
    builder->node(this->animation_packetloss);
  else if (tag == TAG_RAMP_SIGNAL)
    builder->node(this->ramp_signal, signal_quality);
  else
    return false;

  return true;
}
