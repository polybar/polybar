#include "modules/xworkspaces.hpp"

#include <algorithm>
#include <set>
#include <utility>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/meta/base.inl"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace {
  inline bool operator==(const position& a, const position& b) {
    return a.x + a.y == b.x + b.y;
  }
}  // namespace

/**
 * Defines a lexicographical order on position
 */
bool operator<(const position& a, const position& b) {
  return std::make_tuple(a.x, a.y) < std::make_tuple(b.x, b.y);
}

namespace modules {
  template class module<xworkspaces_module>;

  /**
   * Construct module
   */
  xworkspaces_module::xworkspaces_module(const bar_settings& bar, string name_)
      : static_module<xworkspaces_module>(bar, move(name_)), m_connection(connection::make()) {
    m_router->register_action_with_data(EVENT_FOCUS, &xworkspaces_module::action_focus);
    m_router->register_action(EVENT_NEXT, &xworkspaces_module::action_next);
    m_router->register_action(EVENT_PREV, &xworkspaces_module::action_prev);

    // Load config values
    m_pinworkspaces = m_conf.get(name(), "pin-workspaces", m_pinworkspaces);
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);
    m_revscroll = m_conf.get(name(), "reverse-scroll", m_revscroll);

    // Initialize ewmh atoms
    if ((m_ewmh = ewmh_util::initialize()) == nullptr) {
      throw module_error("Failed to initialize ewmh atoms");
    }

    // Check if the WM supports _NET_CURRENT_DESKTOP
    if (!ewmh_util::supports(m_ewmh->_NET_CURRENT_DESKTOP)) {
      throw module_error("The WM does not support _NET_CURRENT_DESKTOP, aborting...");
    }

    // Check if the WM supports _NET_DESKTOP_VIEWPORT
    if (!(m_monitorsupport = ewmh_util::supports(m_ewmh->_NET_DESKTOP_VIEWPORT)) && m_pinworkspaces) {
      throw module_error("The WM does not support _NET_DESKTOP_VIEWPORT (required when `pin-workspaces = true`)");
    }

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, {TAG_LABEL_STATE, TAG_LABEL_MONITOR});

    if (m_formatter->has(TAG_LABEL_MONITOR)) {
      m_monitorlabel = load_optional_label(m_conf, name(), "label-monitor", DEFAULT_LABEL_MONITOR);
    }

    if (m_formatter->has(TAG_LABEL_STATE)) {
      // clang-format off
      m_labels.insert(make_pair(
          desktop_state::ACTIVE, load_optional_label(m_conf, name(), "label-active", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::OCCUPIED, load_optional_label(m_conf, name(), "label-occupied", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::URGENT, load_optional_label(m_conf, name(), "label-urgent", DEFAULT_LABEL_STATE)));
      m_labels.insert(make_pair(
          desktop_state::EMPTY, load_optional_label(m_conf, name(), "label-empty", DEFAULT_LABEL_STATE)));
      // clang-format on
    }

    m_icons = factory_util::shared<iconset>();
    m_icons->add(DEFAULT_ICON, factory_util::shared<label>(m_conf.get(name(), DEFAULT_ICON, ""s)));

    for (const auto& workspace : m_conf.get_list<string>(name(), "icon", {})) {
      auto vec = string_util::tokenize(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], factory_util::shared<label>(vec[1]));
      }
    }

    // Get list of monitors
    m_monitors = randr_util::get_monitors(m_connection, m_connection.root(), false);

    // Get desktop details
    m_desktop_names = get_desktop_names();
    update_current_desktop();

    rebuild_desktops();

    // Get _NET_CLIENT_LIST
    rebuild_clientlist();
    rebuild_desktop_states();
  }

  void xworkspaces_module::update_current_desktop() {
    m_current_desktop = ewmh_util::get_current_desktop();
    if (m_current_desktop < m_desktop_names.size()) {
      m_current_desktop_name = m_desktop_names[m_current_desktop];
    } else {
      throw module_error("The current desktop is outside of the number of desktops reported by the WM");
    }
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xworkspaces_module::handle(const evt::property_notify& evt) {
    std::lock_guard<std::mutex> lock(m_workspace_mutex);

    if (evt->atom == m_ewmh->_NET_CLIENT_LIST || evt->atom == m_ewmh->_NET_WM_DESKTOP) {
      rebuild_clientlist();
      rebuild_desktop_states();
    } else if (evt->atom == m_ewmh->_NET_DESKTOP_NAMES || evt->atom == m_ewmh->_NET_NUMBER_OF_DESKTOPS) {
      m_desktop_names = get_desktop_names();
      rebuild_desktops();
      rebuild_clientlist();
      rebuild_desktop_states();
    } else if (evt->atom == m_ewmh->_NET_CURRENT_DESKTOP) {
      update_current_desktop();
      rebuild_desktop_states();
    } else if (evt->atom == WM_HINTS) {
      rebuild_urgent_hints();
      rebuild_desktop_states();
    } else {
      return;
    }

    broadcast();
  }

  /**
   * Rebuild the list of managed clients
   */
  void xworkspaces_module::rebuild_clientlist() {
    vector<xcb_window_t> newclients = ewmh_util::get_client_list();
    std::sort(newclients.begin(), newclients.end());

    for (auto&& client : newclients) {
      if (m_clients.count(client) == 0) {
        // new client: listen for changes (wm_hint or desktop)
        m_connection.ensure_event_mask(client, XCB_EVENT_MASK_PROPERTY_CHANGE);
      }
    }

    // rebuild entire mapping of clients to desktops
    m_clients.clear();
    m_windows.clear();
    for (auto&& client : newclients) {
      auto desk = ewmh_util::get_desktop_from_window(client);
      m_clients[client] = desk;
      m_windows[desk]++;
    }

    rebuild_urgent_hints();
  }

  /**
   * Goes through all clients and updates the urgent hints on the desktop they are on.
   */
  void xworkspaces_module::rebuild_urgent_hints() {
    m_urgent_desktops.assign(m_desktop_names.size(), false);
    for (auto&& client : ewmh_util::get_client_list()) {
      auto desk = ewmh_util::get_desktop_from_window(client);
      /*
       * EWMH allows for 0xFFFFFFFF to be returned here, which means the window
       * should appear on all desktops.
       *
       * We don't take those windows into account for the urgency hint because
       * it would mark all workspaces as urgent.
       */
      if (desk < m_urgent_desktops.size()) {
        m_urgent_desktops[desk] = m_urgent_desktops[desk] || icccm_util::get_wm_urgency(m_connection, client);
      }
    }
  }

  /**
   * Rebuild the desktop tree
   *
   * This requires m_desktop_names to have an up-to-date value
   */
  void xworkspaces_module::rebuild_desktops() {
    m_viewports.clear();

    /*
     * Stores the _NET_DESKTOP_VIEWPORT hint
     *
     * For WMs that don't support that hint, we store an empty vector
     *
     * If the length of the vector is less than _NET_NUMBER_OF_DESKTOPS
     * all desktops which aren't explicitly assigned a postion will be
     * assigned (0, 0)
     *
     * We use this to map workspaces to viewports, desktop i is at position
     * ws_positions[i].
     */
    vector<position> ws_positions;
    if (m_monitorsupport) {
      ws_positions = ewmh_util::get_desktop_viewports();
    }

    /*
     * Not all desktops were assigned a viewport, add (0, 0) for all missing
     * desktops.
     */
    if (ws_positions.size() < m_desktop_names.size()) {
      auto num_insert = m_desktop_names.size() - ws_positions.size();
      ws_positions.reserve(num_insert);
      std::fill_n(std::back_inserter(ws_positions), num_insert, position{0, 0});
    }

    /*
     * The list of viewports is the set of unique positions in ws_positions
     * Using a set automatically removes duplicates.
     */
    std::set<position> viewports(ws_positions.begin(), ws_positions.end());

    for (auto&& viewport_pos : viewports) {
      /*
       * If pin-workspaces is set, we only add the viewport if it's in the
       * monitor the bar is on.
       * Generally viewport_pos is the same as the top-left coordinate of the
       * monitor but we use `contains` here as a safety in case it isn't exactly.
       */
      if (!m_pinworkspaces || m_bar.monitor->contains(viewport_pos)) {
        auto viewport = make_unique<struct viewport>();
        viewport->state = viewport_state::UNFOCUSED;
        viewport->pos = viewport_pos;

        for (auto&& m : m_monitors) {
          if (m->contains(viewport->pos)) {
            viewport->name = m->name;
            viewport->state = viewport_state::FOCUSED;
          }
        }

        viewport->label = [&] {
          label_t label;
          if (m_monitorlabel) {
            label = m_monitorlabel->clone();
            label->reset_tokens();
            label->replace_token("%name%", viewport->name);
          }
          return label;
        }();

        for (size_t i = 0; i < ws_positions.size(); i++) {
          auto&& ws_pos = ws_positions[i];
          if (ws_pos == viewport_pos) {
            viewport->desktops.emplace_back(make_unique<struct desktop>(i, desktop_state::EMPTY, label_t{}));
          }
        }

        m_viewports.emplace_back(move(viewport));
      }
    }
  }

  /**
   * Update active state of current desktops
   */
  void xworkspaces_module::rebuild_desktop_states() {
    std::set<unsigned int> occupied_desks;
    for (auto&& c : m_clients) {
      occupied_desks.insert(c.second);
    }

    for (auto&& v : m_viewports) {
      for (auto&& d : v->desktops) {
        if (m_urgent_desktops[d->index]) {
          d->state = desktop_state::URGENT;
        } else if (d->index == m_current_desktop) {
          d->state = desktop_state::ACTIVE;
        } else if (occupied_desks.count(d->index) > 0) {
          d->state = desktop_state::OCCUPIED;
        } else {
          d->state = desktop_state::EMPTY;
        }

        d->label = m_labels.at(d->state)->clone();
        d->label->reset_tokens();
        d->label->replace_token("%index%", to_string(d->index + 1));
        d->label->replace_token("%name%", m_desktop_names[d->index]);
        d->label->replace_token("%nwin%", to_string(m_windows[d->index]));
        d->label->replace_token("%icon%", m_icons->get(m_desktop_names[d->index], DEFAULT_ICON)->get());
      }
    }
  }

  vector<string> xworkspaces_module::get_desktop_names() {
    vector<string> names = ewmh_util::get_desktop_names();
    unsigned int desktops_number = ewmh_util::get_number_of_desktops();
    if (desktops_number == names.size()) {
      return names;
    } else if (desktops_number < names.size()) {
      names.erase(names.begin() + desktops_number, names.end());
      return names;
    }
    for (unsigned int i = names.size(); i < desktops_number; i++) {
      names.insert(names.end(), to_string(i + 1));
    }
    return names;
  }

  /**
   * Fetch and parse data
   */
  void xworkspaces_module::update() {}

  /**
   * Generate module output
   */
  string xworkspaces_module::get_output() {
    std::unique_lock<std::mutex> lock(m_workspace_mutex);

    // Get the module output early so that
    // the format prefix/suffix also gets wrapped
    // with the cmd handlers
    string output;
    for (m_index = 0; m_index < m_viewports.size(); m_index++) {
      if (m_index > 0) {
        m_builder->spacing(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += module::get_output();
    }

    if (m_scroll) {
      m_builder->action(mousebtn::SCROLL_DOWN, *this, m_revscroll ? EVENT_NEXT : EVENT_PREV, "");
      m_builder->action(mousebtn::SCROLL_UP, *this, m_revscroll ? EVENT_PREV : EVENT_NEXT, "");
    }

    m_builder->append(output);

    m_builder->action_close();
    m_builder->action_close();

    return m_builder->flush();
  }

  /**
   * Output content as defined in the config
   */
  bool xworkspaces_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_LABEL_MONITOR) {
      if (m_viewports[m_index]->state != viewport_state::NONE) {
        builder->node(m_viewports[m_index]->label);
        return true;
      } else {
        return false;
      }
    } else if (tag == TAG_LABEL_STATE) {
      unsigned int added_states = 0;
      for (auto&& desktop : m_viewports[m_index]->desktops) {
        if (desktop->label.get()) {
          if (m_click && desktop->state != desktop_state::ACTIVE) {
            builder->action(mousebtn::LEFT, *this, EVENT_FOCUS, to_string(desktop->index), desktop->label);
          } else {
            builder->node(desktop->label);
          }
          added_states++;
        }
      }
      return added_states > 0;
    } else {
      return false;
    }
  }

  void xworkspaces_module::action_focus(const string& data) {
    std::lock_guard<std::mutex> lock(m_workspace_mutex);
    focus_desktop(std::strtoul(data.c_str(), nullptr, 10));
  }

  void xworkspaces_module::action_next() {
    focus_direction(true);
  }

  void xworkspaces_module::action_prev() {
    focus_direction(false);
  }

  void xworkspaces_module::focus_direction(bool next) {
    std::lock_guard<std::mutex> lock(m_workspace_mutex);
    vector<unsigned int> indexes;
    for (auto&& viewport : m_viewports) {
      for (auto&& desktop : viewport->desktops) {
        indexes.emplace_back(desktop->index);
      }
    }

    unsigned new_desktop;
    unsigned int current_desktop{ewmh_util::get_current_desktop()};

    if (next) {
      new_desktop = math_util::min<unsigned int>(indexes.back(), current_desktop + 1);
      new_desktop = new_desktop == current_desktop ? indexes.front() : new_desktop;
    } else {
      new_desktop = math_util::max<unsigned int>(indexes.front(), current_desktop - 1);
      new_desktop = new_desktop == current_desktop ? indexes.back() : new_desktop;
    }

    focus_desktop(new_desktop);
  }

  void xworkspaces_module::focus_desktop(unsigned new_desktop) {
    unsigned int current_desktop{ewmh_util::get_current_desktop()};
    if (new_desktop != current_desktop) {
      m_log.info("%s: Requesting change to desktop #%u", name(), new_desktop);
      ewmh_util::change_current_desktop(new_desktop);
    } else {
      m_log.info("%s: Ignoring change to current desktop", name());
    }
  }
}  // namespace modules

POLYBAR_NS_END
