#pragma once

#include <memory>
#include <string>

#include "modules/base.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/ramp.hpp"

namespace modules
{
  DefineModule(BatteryModule, InotifyModule)
  {
    public:
      static const int STATE_UNKNOWN = 1;
      static const int STATE_CHARGING = 2;
      static const int STATE_DISCHARGING = 3;
      static const int STATE_FULL = 4;

    protected:
      static constexpr auto FORMAT_CHARGING = "format-charging";
      static constexpr auto FORMAT_DISCHARGING = "format-discharging";
      static constexpr auto FORMAT_FULL = "format-full";

      static constexpr auto TAG_ANIMATION_CHARGING = "<animation-charging>";
      static constexpr auto TAG_BAR_CAPACITY = "<bar-capacity>";
      static constexpr auto TAG_RAMP_CAPACITY = "<ramp-capacity>";
      static constexpr auto TAG_LABEL_CHARGING = "<label-charging>";
      static constexpr auto TAG_LABEL_DISCHARGING = "<label-discharging>";
      static constexpr auto TAG_LABEL_FULL = "<label-full>";

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
      std::string path_capacity, path_adapter;

      concurrency::Atomic<int> state;
      concurrency::Atomic<int> percentage;
      int full_at;

      void subthread_routine();

    public:
      explicit BatteryModule(std::string name);

      void start();

      bool on_event(InotifyEvent *event);
      std::string get_format();
      bool build(Builder *builder, std::string tag);
  };
}
