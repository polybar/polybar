#include "modules/xwindow.hpp"

#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

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
   *  WM_NAME
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

  string active_window::instance_name() const {
    return icccm_util::get_wm_class(m_connection, m_window).first;
  }

  string active_window::class_name() const {
    return icccm_util::get_wm_class(m_connection, m_window).second;
  }

  /**
   * Construct module
   */
  xwindow_module::xwindow_module(const bar_settings& bar, string name_, const config& config)
      : static_module<xwindow_module>(bar, move(name_), config), m_connection(connection::make()) {
    // Initialize ewmh atoms
    ewmh_util::initialize();

    // Check if the WM supports _NET_ACTIVE_WINDOW
    if (!ewmh_util::supports(_NET_ACTIVE_WINDOW)) {
      throw module_error("The WM does not list _NET_ACTIVE_WINDOW as a supported hint");
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
      reset_active_window();
      update();
    } else if (evt->atom == _NET_CURRENT_DESKTOP) {
      reset_active_window();
      update();
    } else if (evt->atom == _NET_WM_NAME || evt->atom == _NET_WM_VISIBLE_NAME || evt->atom == WM_NAME ||
               evt->atom == WM_CLASS) {
      update();
    } else {
      return;
    }

    broadcast();
  }

  void xwindow_module::reset_active_window() {
    m_active.reset();
  }

  /**
   * Update the currently active window and query its title
   */
  void xwindow_module::update() {
    if (!m_active) {
      xcb_window_t win = ewmh_util::get_active_window();
      if (win != XCB_NONE) {
        m_active = make_unique<active_window>(m_connection, win);
      }
    }

    if (!m_statelabels.empty()) {
      if (m_active) {
        m_label = m_statelabels.at(state::ACTIVE)->clone();
        m_label->reset_tokens();
        m_label->replace_token("%title%", m_active->title());
        m_label->replace_token("%instance%", m_active->instance_name());
        m_label->replace_token("%class%", m_active->class_name());
      } else {
        m_label = m_statelabels.at(state::EMPTY)->clone();
      }
    }
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
} // namespace modules

POLYBAR_NS_END
