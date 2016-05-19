#ifndef _MODULES_MEMORY_HPP_
#define _MODULES_MEMORY_HPP_

#include <atomic>

#include "modules/base.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  DefineModule(MemoryModule, TimerModule)
  {
    const char *TAG_LABEL = "<label>";
    const char *TAG_BAR_USED = "<bar:used>";
    const char *TAG_BAR_FREE = "<bar:free>";

    std::unique_ptr<drawtypes::Bar> bar_used;
    std::unique_ptr<drawtypes::Bar> bar_free;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;

    std::atomic<int> percentage_used;
    std::atomic<int> percentage_free;

    public:
      MemoryModule(const std::string& name);

      bool update();
      bool build(Builder *builder, const std::string& tag);
  };
}

#endif
