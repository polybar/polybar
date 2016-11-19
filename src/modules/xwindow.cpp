#include "modules/xwindow.hpp"
#include "x11/atoms.hpp"
#include "x11/graphics.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Bootstrap the module
   */
  void xwindow_module::setup() {
    connection& conn{configure_connection().create<decltype(conn)>()};

    // Initialize ewmh atoms
    if (!ewmh_util::setup(conn, &m_ewmh)) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_ACTIVE_WINDOW
    if (!ewmh_util::supports(&m_ewmh, _NET_ACTIVE_WINDOW)) {
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
    window root{conn, conn.root()};
    root.ensure_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE);

    // Connect with the event registry
    conn.attach_sink(this, 1);

    // Trigger the initial draw event
    update();
  }

  /**
   * Disconnect from the event registry
   */
  void xwindow_module::teardown() {
    connection& conn{configure_connection().create<decltype(conn)>()};
    conn.detach_sink(this, 1);
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
    xcb_window_t win{ewmh_util::get_active_window(&m_ewmh)};
    string title;

    if (m_active && m_active->match(win)) {
      title = m_active->title(&m_ewmh);
    } else if (win != XCB_NONE) {
      m_active = make_unique<active_window>(win);
      title = m_active->title(&m_ewmh);
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
   * Generate the module output
   */
  string xwindow_module::get_output() {
    m_builder->node(static_module::get_output());
    return m_builder->flush();
  }

  /**
   * Output content as defined in the config
   */
  bool xwindow_module::build(builder* builder, string tag) const {
    if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
