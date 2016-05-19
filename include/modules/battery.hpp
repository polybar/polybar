#ifndef _MODULES_BATTERY_HPP_
#define _MODULES_BATTERY_HPP_

#include <memory>
#include <string>
#include <mutex>

#include "modules/base.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"

namespace modules
{
  enum BatteryState
  {
    UNKNOWN     = 1 << 1,
    CHARGING    = 1 << 2,
    DISCHARGING = 1 << 4,
    FULL        = 1 << 8,
  };

  DefineModule(BatteryModule, InotifyModule)
  {
    const char *FORMAT_CHARGING = "format:charging";
    const char *FORMAT_DISCHARGING = "format:discharging";
    const char *FORMAT_FULL = "format:full";

    const char *TAG_ANIMATION_CHARGING = "<animation:charging>";
    const char *TAG_BAR_CAPACITY = "<bar:capacity>";
    const char *TAG_RAMP_CAPACITY = "<ramp:capacity>";
    const char *TAG_LABEL_CHARGING = "<label:charging>";
    const char *TAG_LABEL_DISCHARGING = "<label:discharging>";
    const char *TAG_LABEL_FULL = "<label:full>";

    // std::mutex ev_mtx;
    // std::condition_variable cv;

    std::unique_ptr<drawtypes::Animation> animation_charging;
    std::unique_ptr<drawtypes::Ramp> ramp_capacity;
    std::unique_ptr<drawtypes::Bar> bar_capacity;
    std::unique_ptr<drawtypes::Label> label_charging;
    std::unique_ptr<drawtypes::Label> label_charging_tokenized;
    std::unique_ptr<drawtypes::Label> label_discharging;
    std::unique_ptr<drawtypes::Label> label_discharging_tokenized;
    std::unique_ptr<drawtypes::Label> label_full;
    std::unique_ptr<drawtypes::Label> label_full_tokenized;

    std::string battery, adapter;
    concurrency::Atomic<int> state;
    // std::atomic<int> state;
    std::atomic<int> percentage;
    int full_at;

    void animation_thread_runner();

    public:
      BatteryModule(const std::string& name);

      bool on_event(InotifyEvent *event);
      std::string get_format();
      bool build(Builder *builder, const std::string& tag);
  };
}

#endif
