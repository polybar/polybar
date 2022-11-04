#include "modules/xwindow.hpp"

#include <iostream>

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
  string xwindow_module::title(xcb_window_t win) const {
    string title;
    if (!(title = ewmh_util::get_wm_name(win)).empty()) {
      return title;
    } else if (!(title = ewmh_util::get_visible_name(win)).empty()) {
      return title;
    } else if (!(title = icccm_util::get_wm_name(m_connection, win)).empty()) {
      return title;
    } else {
      return "";
    }
  }

  string xwindow_module::instance_name(xcb_window_t win) const {
    return icccm_util::get_wm_class(m_connection, win).first;
  }

  string xwindow_module::class_name(xcb_window_t win) const {
    return icccm_util::get_wm_class(m_connection, win).second;
  }

  void xwindow_module::action_focus(const string& data) {
    // This really shouldn't happen, but the bar might unload after user clicks
    if (m_windows.empty())
      return;
    const auto index = std::strtoul(data.c_str(), nullptr, 10) % m_windows.size();
    ewmh_util::focus_window(m_windows[index]);
    m_active_index = index;
  }

  void xwindow_module::action_next() {
    if (m_windows.empty())
      return;
    const auto next_index = (m_active_index + 1) % m_windows.size();
    ewmh_util::focus_window(m_windows[next_index]);
    m_active_index = next_index;
  }

  void xwindow_module::action_prev() {
    if (m_windows.empty())
      return;
    const auto prev_index = (m_active_index + m_windows.size() - 1) % m_windows.size();
    ewmh_util::focus_window(m_windows[prev_index]);
    m_active_index = prev_index;
  }

  /**
   * Construct module
   */
  xwindow_module::xwindow_module(const bar_settings& bar, string name_)
      : static_module<xwindow_module>(bar, move(name_)), m_connection(connection::make()) {
    // Initialize ewmh atoms
    ewmh_util::initialize();
    m_router->register_action_with_data(EVENT_FOCUS, [this](const std::string& data) { action_focus(data); });
    m_router->register_action(EVENT_NEXT, [this]() { action_next(); });
    m_router->register_action(EVENT_PREV, [this]() { action_prev(); });

    // Check if the WM supports _NET_ACTIVE_WINDOW
    if (!ewmh_util::supports(_NET_ACTIVE_WINDOW)) {
      throw module_error("The WM does not list _NET_ACTIVE_WINDOW as a supported hint");
    }

    // Load config values
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);
    m_revscroll = m_conf.get(name(), "reverse-scroll", m_revscroll);

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_statelabels.emplace(state::DEFAULT, load_optional_label(m_conf, name(), "label", ""));
      m_statelabels.emplace(state::ACTIVE, load_optional_label(m_conf, name(), "label-active", "%title%"));
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
      m_labels.clear();
      if (m_active) {
        m_windows.clear();
        const auto current_desktop = ewmh_util::get_current_desktop();
        const auto clients = ewmh_util::get_client_list();
        for (size_t i = 0; i < clients.size(); ++i) {
          auto&& client = clients[i];
          const auto desktop = ewmh_util::get_desktop_from_window(client);
          if (desktop != current_desktop)
            continue;
          m_windows.push_back(client);
          if (m_active && m_active->match(client)) {
            m_active_index = i;
            m_labels.push_back(m_statelabels.at(state::ACTIVE)->clone());
          } else {
            m_labels.push_back(m_statelabels.at(state::DEFAULT)->clone());
          }
          auto label = m_labels.back();
          label->reset_tokens();
          label->replace_token("%title%", title(client));
          label->replace_token("%instance%", instance_name(client));
          label->replace_token("%class%", class_name(client));
        }
      } else {
        m_labels.push_back(m_statelabels.at(state::EMPTY)->clone());
      }
    }
  }

  /**
   * Output content as defined in the config
   */
  bool xwindow_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL) {
      if (m_scroll) {
        const auto down = m_revscroll ? EVENT_PREV : EVENT_NEXT;
        const auto up = m_revscroll ? EVENT_NEXT : EVENT_PREV;
        builder->action(mousebtn::SCROLL_DOWN, *this, down, "");
        builder->action(mousebtn::SCROLL_UP, *this, up, "");
      }
      // Labels are inserted in reverse order so that they appear in the normal order on the bar
      for (size_t i = m_labels.size(); i > 0; --i) {
        auto& label = m_labels[i - 1];
        if (m_click) {
          builder->action(mousebtn::LEFT, *this, EVENT_FOCUS, to_string(i - 1), label);
        } else {
          builder->node(label);
        }
      }
      return true;
    }
    return false;
  }
} // namespace modules

POLYBAR_NS_END
