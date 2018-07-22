#pragma once

#include "modules/meta/static_module.hpp"

POLYBAR_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    explicit text_module(const bar_settings&, string);

    void update() {}

    string get_output();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_CONTENT{"<content>"};

    label_t m_label;
  };
}

POLYBAR_NS_END
