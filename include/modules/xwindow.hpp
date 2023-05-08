#pragma once

#include "modules/meta/event_handler.hpp"
#include "modules/meta/static_module.hpp"
#include "modules/meta/types.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"
#include "x11/window.hpp"

POLYBAR_NS

class connection;

namespace modules {
  class active_window : public non_copyable_mixin, public non_movable_mixin {
   public:
    explicit active_window(xcb_connection_t* conn, xcb_window_t win);
    ~active_window();

    bool match(xcb_window_t win) const;
    string title() const;
    string instance_name() const;
    string class_name() const;

   private:
    xcb_connection_t* m_connection{nullptr};
    xcb_window_t m_window{XCB_NONE};
  };

  /**
   * Module used to display information about the
   * currently active X window.
   */
  class xwindow_module : public static_module<xwindow_module>, public event_handler<evt::property_notify> {
   public:
    enum class state { NONE, ACTIVE, EMPTY };
    explicit xwindow_module(const bar_settings&, string, const config&);

    void update();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = XWINDOW_TYPE;

   protected:
    void handle(const evt::property_notify& evt) override;

    void reset_active_window();

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    connection& m_connection;
    unique_ptr<active_window> m_active;
    map<state, label_t> m_statelabels;
    label_t m_label;
  };
} // namespace modules

POLYBAR_NS_END
