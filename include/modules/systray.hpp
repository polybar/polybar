#if DEBUG
#pragma once

#include "modules/meta/static_module.hpp"

POLYBAR_NS

class connection;

namespace modules {
  /**
   * Module used to display information about the
   * currently active X window.
   */
  class systray_module : public static_module<systray_module> {
   public:
    explicit systray_module(const bar_settings&, string);

    void update();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "internal/systray";

    static constexpr auto EVENT_TOGGLE = "toggle";

   protected:
    bool input(const string& action, const string& data);

   private:

    static constexpr const char* TAG_LABEL_TOGGLE{"<label-toggle>"};
    static constexpr const char* TAG_TRAY_CLIENTS{"<tray-clients>"};

    connection& m_connection;
    label_t m_label;

    bool m_hidden{false};
  };
}  // namespace modules

POLYBAR_NS_END
#endif
