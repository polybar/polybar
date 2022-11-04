#pragma once

#include "modules/meta/event_handler.hpp"
#include "modules/meta/static_module.hpp"
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
    static constexpr auto EVENT_FOCUS = "focus";
    static constexpr auto EVENT_NEXT = "next";
    static constexpr auto EVENT_PREV = "prev";

    enum class state { NONE, DEFAULT, ACTIVE, EMPTY };
    explicit xwindow_module(const bar_settings&, string);

    void update();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "internal/xwindow";

   protected:
    void handle(const evt::property_notify& evt) override;

    void reset_active_window();

    string title(xcb_window_t win) const;
    string instance_name(xcb_window_t win) const;
    string class_name(xcb_window_t win) const;

    void action_focus(const string& data);
    void action_next();
    void action_prev();

   private:
    static constexpr const char* TAG_LABEL{"<label>"};

    connection& m_connection;
    unique_ptr<active_window> m_active;
    map<state, label_t> m_statelabels;
    vector<xcb_window_t> m_windows;
    size_t m_active_index;
    vector<label_t> m_labels;
    bool m_click{false};
    bool m_scroll{false};
    bool m_revscroll{false};
  };
} // namespace modules

POLYBAR_NS_END
