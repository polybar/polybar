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
  /**
   * Wrapper used to update the event mask of the
   * currently active to enable title tracking
   */
  class active_window {
   public:
    explicit active_window(xcb_window_t win)
        : m_connection(configure_connection().create<decltype(m_connection)>()), m_window(m_connection, win) {
      try {
        m_window.change_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE);
      } catch (const xpp::x::error::window& err) {
      }
    }

    ~active_window() {
      try {
        m_window.change_event_mask(XCB_EVENT_MASK_NO_EVENT);
      } catch (const xpp::x::error::window& err) {
      }
    }

    /**
     * Check if current window matches passed value
     */
    bool match(const xcb_window_t win) const {
      return m_window == win;
    }

    /**
     * Get the title by returning the first non-empty value of:
     *  _NET_WM_VISIBLE_NAME
     *  _NET_WM_NAME
     */
    string title(xcb_ewmh_connection_t* ewmh) {
      string title;

      if (!(title = ewmh_util::get_visible_name(ewmh, m_window)).empty()) {
        return title;
      } else if (!(title = icccm_util::get_wm_name(m_connection, m_window)).empty()) {
        return title;
      } else {
        return "";
      }
    }

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
    using static_module::static_module;

    void setup();
    void teardown();
    void handle(const evt::property_notify& evt);
    void update();
    string get_output();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    ewmh_connection_t m_ewmh;
    unique_ptr<active_window> m_active;
    label_t m_label;
  };
}

POLYBAR_NS_END
