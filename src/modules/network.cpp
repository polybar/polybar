#include <thread>

#include "lemonbuddy.hpp"
#include "modules/network.hpp"
#include "services/logger.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/proc.hpp"
#include "utils/timer.hpp"

using namespace modules;

// TODO: Add up-/download speed (check how ifconfig read the bytes)

NetworkModule::NetworkModule(const std::string& name_) : TimerModule(name_, 1s)
{
  this->interval = std::chrono::duration<double>(
    config::get<float>(name(), "interval", 1));
  this->connectivity_test_interval = config::get<int>(
    name(), "connectivity_test_interval", 0);
  this->interface = config::get<std::string>(name(), "interface");
  this->connected = false;

  this->formatter->add(FORMAT_CONNECTED, TAG_LABEL_CONNECTED, { TAG_RAMP_SIGNAL, TAG_LABEL_CONNECTED });
  this->formatter->add(FORMAT_DISCONNECTED, TAG_LABEL_DISCONNECTED, { TAG_LABEL_DISCONNECTED });

  if (this->formatter->has(TAG_RAMP_SIGNAL, FORMAT_CONNECTED))
    this->ramp_signal = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP_SIGNAL));
  if (this->formatter->has(TAG_LABEL_CONNECTED, FORMAT_CONNECTED))
    this->label_connected = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_CONNECTED), "%ifname% %local_ip%");

  if (this->formatter->has(TAG_LABEL_DISCONNECTED, FORMAT_DISCONNECTED)) {
    this->label_disconnected = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_DISCONNECTED), "");
    this->label_disconnected->replace_token("%ifname%", this->interface);
  }

  if (this->connectivity_test_interval > 0) {
    this->formatter->add(FORMAT_PACKETLOSS, "", { TAG_ANIMATION_PACKETLOSS, TAG_LABEL_PACKETLOSS });

    if (this->formatter->has(TAG_LABEL_PACKETLOSS, FORMAT_PACKETLOSS))
      this->label_packetloss = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_PACKETLOSS), "%ifname% %local_ip%");
    if (this->formatter->has(TAG_ANIMATION_PACKETLOSS, FORMAT_PACKETLOSS))
      this->animation_packetloss = drawtypes::get_config_animation(name(), get_tag_name(TAG_ANIMATION_PACKETLOSS));
  }

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

NetworkModule::~NetworkModule()
{
  std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
  // if (this->t_animation.joinable())
  //   this->t_animation.join();
}

// void NetworkModule::dispatch()
// {
//   this->EventModule::dispatch();
//
//   // if (this->animation_packetloss)
//   //   this->t_animation = std::thread(&NetworkModule::animation_thread_runner, this);
// }

// bool NetworkModule::has_event()
// {
//   std::this_thread::sleep_for(this->interval);
//   return true;
// }

// void NetworkModule::animation_thread_runner()
// {
//   while (this->enabled()) {
//     std::unique_lock<std::mutex> lck(this->mtx);
//
//     if (this->connected && this->conseq_packetloss)
//       this->Module::notify_change();
//     else
//       this->cv.wait(lck, [&]{
//         return this->connected && this->conseq_packetloss; });
//
//     std::this_thread::sleep_for(std::chrono::duration<double>(
//       float(this->animation_packetloss->get_framerate()) / 1000));
//   }
// }

bool NetworkModule::update()
{
  std::string ip, essid, linkspeed;
  int signal_quality = 0;

  if (this->wireless_network) {
    try {
      ip = this->wireless_network->get_ip();
    } catch (net::NetworkException &e) {
      get_logger()->debug(e.what());
    }

    try {
      essid = this->wireless_network->get_essid();
      signal_quality = this->wireless_network->get_signal_quality();
    } catch (net::WirelessNetworkException &e) {
      get_logger()->debug(e.what());
    }

    this->connected = this->wireless_network->connected();
    this->signal_quality = signal_quality;

    if (this->connectivity_test_interval > 0 && this->connected && this->counter++ % PING_EVERY_NTH_UPDATE == 0)
      this->conseq_packetloss = !this->wireless_network->test();
  } else if (this->wired_network) {
    try {
      ip = this->wired_network->get_ip();
    } catch (net::NetworkException &e) {
      get_logger()->debug(e.what());
    }

    linkspeed = this->wired_network->get_link_speed();

    this->connected = this->wired_network->connected();

    if (this->connectivity_test_interval > 0 && this->connected && this->counter++ % PING_EVERY_NTH_UPDATE == 0)
      this->conseq_packetloss = !this->wired_network->test();
  }

  if (this->label_connected || this->label_packetloss) {
    auto replace_tokens = [&](std::unique_ptr<drawtypes::Label> &label){
      label->replace_token("%ifname%", this->interface);
      label->replace_token("%local_ip%", ip);

      if (this->wired_network) {
        label->replace_token("%linkspeed%", linkspeed);
      } else if (this->wireless_network) {
        // label->replace_token("%essid%", essid);
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

  // this->cv.notify_all();

  return true;
}

std::string NetworkModule::get_format()
{
  if (!this->connected)
    return FORMAT_DISCONNECTED;
  else if (this->conseq_packetloss)
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
