#include "modules/xwindow.hpp"
#include "drawtypes/label.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/graphics.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/static_module.inl"

POLYBAR_NS

namespace modules {
  template class module<xwindow_module>;
  template class static_module<xwindow_module>;

  /**
   * Wrapper used to update the event mask of the
   * currently active to enable title tracking
   */
  active_window::active_window(xcb_window_t win)
      : m_connection(make_connection()), m_window(m_connection, win) {
    try {
      m_window.change_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE);
    } catch (const xpp::x::error::window& err) {
    }
  }

  /**
   * Deconstruct window object
   */
  active_window::~active_window() {
    try {
      m_window.change_event_mask(XCB_EVENT_MASK_NO_EVENT);
    } catch (const xpp::x::error::window& err) {
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
   *  _NET_WM_VISIBLE_NAME
   *  _NET_WM_NAME
   */
  string active_window::title(xcb_ewmh_connection_t* ewmh) const {
    string title;

    if (!(title = ewmh_util::get_visible_name(ewmh, m_window)).empty()) {
      return title;
    } else if (!(title = ewmh_util::get_wm_name(ewmh, m_window)).empty()) {
      return title;
    } else if (!(title = icccm_util::get_wm_name(m_connection, m_window)).empty()) {
      return title;
    } else {
      return "";
    }
  }

  /**
   * Construct module
   */
  xwindow_module::xwindow_module(const bar_settings& bar, const logger& logger, const config& config, string name)
      : static_module<xwindow_module>(bar, logger, config, name)
      , m_connection(make_connection()) {}

  /**
   * Bootstrap the module
   */
  void xwindow_module::setup() {
    // Initialize ewmh atoms
    if ((m_ewmh = ewmh_util::initialize()) == nullptr) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_ACTIVE_WINDOW
    if (!ewmh_util::supports(m_ewmh.get(), _NET_ACTIVE_WINDOW)) {
      throw module_error("The WM does not list _NET_ACTIVE_WINDOW as a supported hint");
    }

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%title%");
    }

    // No need to setup X components if we can't show the title
    if (!m_label || !m_label->has_token("%title%")) {
      return;
    }

    // Make sure we get notified when root properties change
    m_connection.ensure_event_mask(m_connection.root(), XCB_EVENT_MASK_PROPERTY_CHANGE);

    // Connect with the event registry
    m_connection.attach_sink(this, SINK_PRIORITY_MODULE);

    // Trigger the initial draw event
    update();
  }

  /**
   * Disconnect from the event registry
   */
  void xwindow_module::teardown() {
    m_connection.detach_sink(this, SINK_PRIORITY_MODULE);
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xwindow_module::handle(const evt::property_notify& evt) {
    if (evt->atom == _NET_ACTIVE_WINDOW) {
      update();
    } else if (evt->atom == _NET_CURRENT_DESKTOP) {
      update();
    } else if (evt->atom == _NET_WM_VISIBLE_NAME) {
      update();
    } else if (evt->atom == _NET_WM_NAME) {
      update();
    } else {
      return;
    }
  }

  /**
   * Update the currently active window and query its title
   */
  void xwindow_module::update() {
    xcb_window_t win{ewmh_util::get_active_window(m_ewmh.get())};
    string title;

    if (m_active && m_active->match(win)) {
      title = m_active->title(m_ewmh.get());
    } else if (win != XCB_NONE) {
      m_active = make_unique<active_window>(win);
      title = m_active->title(m_ewmh.get());
    } else {
      m_active.reset();
    }

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%title%", title);
    }

    // Emit notification to trigger redraw
    broadcast();
  }

  /**
   * Output content as defined in the config
   */
  bool xwindow_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
