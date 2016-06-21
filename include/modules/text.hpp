#pragma once

#include "modules/base.hpp"

namespace modules
{
  DefineModule(TextModule, StaticModule)
  {
    static constexpr auto FORMAT = "content";

    public:
      explicit TextModule(std::string name);

      std::string get_format();
      std::string get_output();
  };
}
