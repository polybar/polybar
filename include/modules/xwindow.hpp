#pragma once

#include <bitset>

#include "components/config.hpp"
#include "modules/meta/static_module.hpp"
#include "x11/events.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"
#include "x11/window.hpp"

POLYBAR_NS

class connection;

namespace modules {
  class active_window {
   public:
    explicit active_window(xcb_window_t win);
    ~active_window();
    bool match(const xcb_window_t win) const;
    string title(xcb_ewmh_connection_t* ewmh) const;

   private:
    connection& m_connection;
    window m_window{m_connection};
  };

  /**
   * Module used to display information about the
   * currently active X window.
   */
  class xwindow_module : public static_module<xwindow_module>, public xpp::event::sink<evt::property_notify> {
   public:
    explicit xwindow_module(const bar_settings&, string);

    void setup();
    void teardown();
    void handle(const evt::property_notify& evt);
    void update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    connection& m_connection;
    ewmh_connection_t m_ewmh;
    unique_ptr<active_window> m_active;
    label_t m_label;
  };
}

POLYBAR_NS_END
