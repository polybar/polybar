#ifndef _MODULES_DATE_HPP_
#define _MODULES_DATE_HPP_

#include <string>

#include "modules/base.hpp"

namespace modules
{
  DefineModule(DateModule, TimerModule)
  {
    const char *TAG_DATE = "<date>";

    const char *EVENT_TOGGLE = "datetoggle";

    std::unique_ptr<Builder> builder;

    std::string date;
    std::string date_detailed;

    char date_str[256];
    bool detailed = false;

    public:
      DateModule(const std::string& name);

      bool update();
      bool build(Builder *builder, const std::string& tag);

      std::string get_output();
      bool handle_command(const std::string& cmd);
  };
}

#endif
