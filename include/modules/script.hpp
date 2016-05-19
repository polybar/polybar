#ifndef _MODULES_SCRIPT_HPP_
#define _MODULES_SCRIPT_HPP_

#include "modules/base.hpp"
#include "services/command.hpp"

namespace modules
{
  DefineModule(ScriptModule, TimerModule)
  {
    const char *TAG_OUTPUT = "<output>";

    std::unique_ptr<Builder> builder;

    std::string exec;
    std::string click_left;
    std::string click_middle;
    std::string click_right;
    std::string scroll_up;
    std::string scroll_down;

    std::string output;
    std::atomic<int> counter;

    protected:

    public:
      ScriptModule(const std::string& name);

      bool update();
      bool build(Builder *builder, const std::string& tag);
      std::string get_output();
  };
}

#endif
