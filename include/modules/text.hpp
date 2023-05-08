#pragma once

#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"

POLYBAR_NS

namespace modules {
  class text_module : public static_module<text_module> {
   public:
    explicit text_module(const bar_settings&, string, const config&);

    void update() {}
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = TEXT_TYPE;

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    label_t m_label;

    string m_format{DEFAULT_FORMAT};
  };
} // namespace modules

POLYBAR_NS_END
