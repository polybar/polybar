#include "modules/xwindow.hpp"
#include "drawtypes/label.hpp"
#include "utils/factory.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<xwindow_module>;

  /**
   * Wrapper used to update the event mask of the
   * currently active to enable title tracking
   */
  active_window::active_window(xcb_connection_t* conn, xcb_window_t win) : m_connection(conn), m_window(win) {
    if (m_window != XCB_NONE) {
      const unsigned int mask{XCB_EVENT_MASK_PROPERTY_CHANGE};
      xcb_change_window_attributes(m_connection, m_window, XCB_CW_EVENT_MASK, &mask);
    }
  }

  /**
   * Deconstruct window object
   */
  active_window::~active_window() {
    if (m_window != XCB_NONE) {
      const unsigned int mask{XCB_EVENT_MASK_NO_EVENT};
      xcb_change_window_attributes(m_connection, m_window, XCB_CW_EVENT_MASK, &mask);
    }
  }

  /**
   * Check if current window matches passed value
   */
  bool active_window::match(const xcb_window_t win) const {
    return m_window == win;
  }

  /**
   * Get the title by returning the first non-empty value of:
   *  _NET_WM_NAME
   *  _NET_WM_VISIBLE_NAME
   */
  string active_window::title() const {
    string title;

    if (!(title = ewmh_util::get_wm_name(m_window)).empty()) {
      return title;
    } else if (!(title = ewmh_util::get_visible_name(m_window)).empty()) {
      return title;
    } else if (!(title = icccm_util::get_wm_name(m_connection, m_window)).empty()) {
      return title;
    } else {
      return "";
    }
  }

  xcb_window_t active_window::get_window() const {
    return m_window;
  }

  /**
   * Construct module
   */
  xwindow_module::xwindow_module(const bar_settings& bar, string name_)
      : static_module<xwindow_module>(bar, move(name_)), m_connection(connection::make()) {
    // Load config values
    if ((m_pinoutput = m_conf.get(name(), "pin-output", m_pinoutput))) {
      m_showlast = m_conf.get(name(), "pin-show-last", m_showlast);
    }

    // Initialize ewmh atoms
    if ((m_ewmh = ewmh_util::initialize()) == nullptr) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_ACTIVE_WINDOW
    if (!ewmh_util::supports(_NET_ACTIVE_WINDOW)) {
      throw module_error("The WM does not list _NET_ACTIVE_WINDOW as a supported hint");
    }

    // Check if the WM supports _NET_DESKTOP_VIEWPORT
    if (!ewmh_util::supports(m_ewmh->_NET_DESKTOP_VIEWPORT) && m_pinoutput) {
      throw module_error("The WM does not support _NET_DESKTOP_VIEWPORT (required when `pin-output = true`)");
    }

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_statelabels.emplace(state::ACTIVE, load_optional_label(m_conf, name(), "label", "%title%"));
      m_statelabels.emplace(state::EMPTY, load_optional_label(m_conf, name(), "label-empty", ""));
    }
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xwindow_module::handle(const evt::property_notify& evt) {
    if (evt->atom == _NET_ACTIVE_WINDOW) {
      update(true);
    } else if (evt->atom == _NET_CURRENT_DESKTOP) {
      update(true);
    } else if (evt->atom == _NET_WM_VISIBLE_NAME) {
      update();
    } else if (evt->atom == _NET_WM_NAME) {
      update();
    } else {
      return;
    }

    broadcast();
  }

  /**
   * Update the currently active window and query its title
   */
  void xwindow_module::update(bool force) {
    std::lock(m_buildlock, m_updatelock);
    std::lock_guard<std::mutex> guard_a(m_buildlock, std::adopt_lock);
    std::lock_guard<std::mutex> guard_b(m_updatelock, std::adopt_lock);

    xcb_window_t win;

    if (force) {
      m_active.reset();
    }

    if (!m_active && (win = ewmh_util::get_active_window()) != XCB_NONE) {
      m_active = make_unique<active_window>(m_connection, win);
    }

    if (m_active) {
      m_label = m_statelabels.at(state::ACTIVE)->clone();
      m_label->reset_tokens();

      if (m_pinoutput) {
        if (active_window_on_monitor(m_active->get_window())) {
          m_label->replace_token("%title%", m_active->title());
          m_lasttitle = m_active->title();
          return;
        } else {
          m_label->replace_token("%title%", m_showlast ? m_lasttitle : "");
        }
      } else {
        m_label->replace_token("%title%", m_active->title());
      }
    } else {
      m_label = m_statelabels.at(state::EMPTY)->clone();
    }
  }

  bool xwindow_module::active_window_on_monitor(xcb_window_t window) const {
    auto desktop_names = ewmh_util::get_desktop_names();
    auto viewports = ewmh_util::get_desktop_viewports();
    viewports.erase(std::unique(viewports.begin(), viewports.end(), [](auto& a, auto& b) { return a == b; }), viewports.end());

    unsigned int step = desktop_names.size() / viewports.size();
    unsigned int offset{0};

    for (unsigned int i = 0; i < viewports.size(); i++) {
      if (m_bar.monitor->match(viewports[i])) {
        for (unsigned int j = offset; j < offset + step; j++) {
          if (ewmh_util::get_desktop_from_window(window) == j) {
            return true;
          }
        }
      }
      offset += step;
    }
    return false;
  }
  /**
   * Output content as defined in the config
   */
  bool xwindow_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL && m_label) {
      builder->node(m_label);
      return true;
    }
    return false;
  }
}

POLYBAR_NS_END
