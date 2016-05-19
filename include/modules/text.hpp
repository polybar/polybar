#ifndef _MODULES_TEXT_HPP_
#define _MODULES_TEXT_HPP_

#include "modules/base.hpp"

namespace modules
{
  DefineModule(TextModule, StaticModule)
  {
    const char *FORMAT = "content";

    public:
      TextModule(const std::string& name) throw(UndefinedFormat);

      std::string get_format();
      std::string get_output();
  };
}

#endif
