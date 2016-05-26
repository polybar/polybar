#pragma once

#include <atomic>

#include "modules/base.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(MemoryModule, TimerModule)
  {
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_USED = "<bar-used>";
    static constexpr auto TAG_BAR_FREE = "<bar-free>";

    std::unique_ptr<drawtypes::Bar> bar_used;
    std::unique_ptr<drawtypes::Bar> bar_free;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;

    std::atomic<int> percentage_used;
    std::atomic<int> percentage_free;

    public:
      explicit MemoryModule(const std::string& name);

      bool update();
      bool build(Builder *builder, const std::string& tag);
  };
}
