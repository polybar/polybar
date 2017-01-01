#if DEBUG
#pragma once

#include "modules/meta/static_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

class connection;

namespace modules {
  /**
   * Module used to display information about the
   * currently active X window.
   */
  class systray_module : public static_module<systray_module>, public input_handler {
   public:
    explicit systray_module(const bar_settings&, string);

    void update();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool input(string&& cmd);

   private:
    static constexpr const char* EVENT_TOGGLE{"systray-toggle"};

    static constexpr const char* TAG_LABEL_TOGGLE{"<label-toggle>"};
    static constexpr const char* TAG_TRAY_CLIENTS{"<tray-clients>"};

    connection& m_connection;
    label_t m_label;

    bool m_hidden{false};
  };
}

POLYBAR_NS_END
#endif
