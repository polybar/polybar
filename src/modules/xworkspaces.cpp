#include "modules/xworkspaces.hpp"
#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/static_module.inl"

POLYBAR_NS

namespace modules {
  template class module<xworkspaces_module>;
  template class static_module<xworkspaces_module>;

  /**
   * Construct module
   */
  xworkspaces_module::xworkspaces_module(
      const bar_settings& bar, const logger& logger, const config& config, string name)
      : static_module<xworkspaces_module>(bar, logger, config, name)
      , m_connection(configure_connection().create<connection&>()) {}

  /**
   * Bootstrap the module
   */
  void xworkspaces_module::setup() {
    connection& conn{configure_connection().create<decltype(conn)>()};

    // Initialize ewmh atoms
    if ((m_ewmh = ewmh_util::initialize()) == nullptr) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_CURRENT_DESKTOP
    if (!ewmh_util::supports(m_ewmh.get(), _NET_CURRENT_DESKTOP)) {
      throw module_error("The WM does not list _NET_CURRENT_DESKTOP as a supported hint");
    }

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_labels.insert(
          make_pair(desktop_state::ACTIVE, load_optional_label(m_conf, name(), "label-active", DEFAULT_LABEL)));
      m_labels.insert(
          make_pair(desktop_state::OCCUPIED, load_optional_label(m_conf, name(), "label-occupied", DEFAULT_LABEL)));
      m_labels.insert(
          make_pair(desktop_state::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_LABEL)));
      m_labels.insert(
          make_pair(desktop_state::EMPTY, load_optional_label(m_conf, name(), "label-empty", DEFAULT_LABEL)));
    }

    m_icons = make_shared<iconset>();
    m_icons->add(DEFAULT_ICON, make_shared<label>(m_conf.get<string>(name(), DEFAULT_ICON, "")));

    for (const auto& workspace : m_conf.get_list<string>(name(), "icon", {})) {
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], make_shared<label>(vec[1]));
      }
    }

    // Make sure we get notified when root properties change
    window{conn, conn.root()}.ensure_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE);

    // Connect with the event registry
    conn.attach_sink(this, 1);

    // Get desktops
    rebuild_desktops();
    set_current_desktop();

    // Trigger the initial draw event
    update();
  }

  /**
   * Disconnect from the event registry
   */
  void xworkspaces_module::teardown() {
    connection& conn{configure_connection().create<decltype(conn)>()};
    conn.detach_sink(this, 1);
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xworkspaces_module::handle(const evt::property_notify& evt) {
    if (evt->atom == m_ewmh->_NET_DESKTOP_NAMES && !m_throttle.throttle(evt->time)) {
      rebuild_desktops();
    } else if (evt->atom == m_ewmh->_NET_CURRENT_DESKTOP && !m_throttle.throttle(evt->time)) {
      set_current_desktop();
    } else {
      return;
    }

    update();

    // m_log.err("%lu %s", evt->time, m_connection.get_atom_name(evt->atom).name());
  }

  /**
   * Update the currently active window and query its title
   */
  void xworkspaces_module::update() {
    size_t desktop_index{0};

    for (const auto& desktop : m_desktops) {
      switch (desktop->state) {
        case desktop_state::ACTIVE:
          desktop->label = m_labels[desktop_state::ACTIVE]->clone();
          break;
        case desktop_state::EMPTY:
          desktop->label = m_labels[desktop_state::EMPTY]->clone();
          break;
        case desktop_state::URGENT:
          desktop->label = m_labels[desktop_state::URGENT]->clone();
          break;
        case desktop_state::OCCUPIED:
          desktop->label = m_labels[desktop_state::URGENT]->clone();
          break;
        case desktop_state::NONE:
          continue;
          break;
      }

      desktop->label->reset_tokens();
      desktop->label->replace_token("%name%", desktop->name);
      desktop->label->replace_token("%icon%", m_icons->get(desktop->name, DEFAULT_ICON)->get());
      desktop->label->replace_token("%index%", to_string(++desktop_index));
    }

    // Emit notification to trigger redraw
    broadcast();
  }

  /**
   * Generate module output
   */
  string xworkspaces_module::get_output() {
    string output;
    for (m_index = 0; m_index < m_desktops.size(); m_index++) {
      if (m_index > 0) {
        m_builder->space(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += static_module::get_output();
    }
    return output;
  }

  /**
   * Output content as defined in the config
   */
  bool xworkspaces_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL && m_desktops[m_index]->state != desktop_state::NONE) {
      builder->node(m_desktops[m_index]->label);
    } else {
      return false;
    }
    return true;
  }

  /**
   * Rebuild the list of desktops
   */
  void xworkspaces_module::rebuild_desktops() {
    m_desktops.clear();

    for (auto&& name : ewmh_util::get_desktop_names(m_ewmh.get())) {
      m_desktops.emplace_back(make_unique<desktop>(move(name), desktop_state::EMPTY));
    }
  }

  /**
   * Flag the desktop that is currently active
   */
  void xworkspaces_module::set_current_desktop() {
    auto current = ewmh_util::get_current_desktop(m_ewmh.get());

    for (auto&& desktop : m_desktops) {
      desktop->state = desktop_state::EMPTY;
    }

    if (m_desktops.size() > current) {
      m_desktops[current]->state = desktop_state::ACTIVE;
    }
  }
}

POLYBAR_NS_END
