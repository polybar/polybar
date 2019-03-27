#include <algorithm>
#include <utility>

#include "drawtypes/iconset.hpp"
#include "drawtypes/label.hpp"
#include "modules/xworkspaces.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace {
  inline bool operator==(const position& a, const position& b) {
    return a.x + a.y == b.x + b.y;
  }
}

namespace modules {
  template class module<xworkspaces_module>;

  /**
   * Construct module
   */
  xworkspaces_module::xworkspaces_module(const bar_settings& bar, string name_)
      : static_module<xworkspaces_module>(bar, move(name_)), m_connection(connection::make()) {
    // Load config values
    m_pinworkspaces = m_conf.get(name(), "pin-workspaces", m_pinworkspaces);
    m_click = m_conf.get(name(), "enable-click", m_click);
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);

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
      auto vec = string_util::split(workspace, ';');
      if (vec.size() == 2) {
        m_icons->add(vec[0], factory_util::shared<label>(vec[1]));
      }
    }

    // Get list of monitors
    m_monitors = randr_util::get_monitors(m_connection, m_connection.root(), false);

    // Get desktop details
    m_desktop_names = get_desktop_names();
    m_current_desktop = ewmh_util::get_current_desktop();
    m_current_desktop_name = m_desktop_names[m_current_desktop];

    rebuild_desktops();
    rebuild_desktop_states();

    // Get _NET_CLIENT_LIST
    rebuild_clientlist();
  }

  /**
   * Handler for XCB_PROPERTY_NOTIFY events
   */
  void xworkspaces_module::handle(const evt::property_notify& evt) {
    if (evt->atom == m_ewmh->_NET_CLIENT_LIST) {
      rebuild_clientlist();
    } else if (evt->atom == m_ewmh->_NET_DESKTOP_NAMES || evt->atom == m_ewmh->_NET_NUMBER_OF_DESKTOPS) {
      m_desktop_names = get_desktop_names();
      rebuild_desktops();
      rebuild_desktop_states();
    } else if (evt->atom == m_ewmh->_NET_CURRENT_DESKTOP) {
      m_current_desktop = ewmh_util::get_current_desktop();
      m_current_desktop_name = m_desktop_names[m_current_desktop];
      rebuild_desktop_states();
    } else if (evt->atom == WM_HINTS) {
      if (icccm_util::get_wm_urgency(m_connection, evt->window)) {
        set_desktop_urgent(evt->window);
      }
    } else {
      return;
    }

    if (m_timer.allow(evt->time)) {
      broadcast();
    }
  }

  /**
   * Rebuild the list of managed clients
   */
  void xworkspaces_module::rebuild_clientlist() {
    vector<xcb_window_t> clients = ewmh_util::get_client_list();
    vector<xcb_window_t> diff;
    std::sort(clients.begin(), clients.end());
    std::sort(m_clientlist.begin(), m_clientlist.end());

    if (m_clientlist.size() > clients.size()) {
      std::set_difference(
          m_clientlist.begin(), m_clientlist.end(), clients.begin(), clients.end(), back_inserter(diff));
      for (auto&& win : diff) {
        // untrack window
        m_clientlist.erase(std::remove(m_clientlist.begin(), m_clientlist.end(), win), m_clientlist.end());
      }
    } else {
      std::set_difference(
          clients.begin(), clients.end(), m_clientlist.begin(), m_clientlist.end(), back_inserter(diff));
      for (auto&& win : diff) {
        // listen for wm_hint (urgency) changes
        m_connection.ensure_event_mask(win, XCB_EVENT_MASK_PROPERTY_CHANGE);
        // track window
        m_clientlist.emplace_back(win);
      }
    }
  }

  /**
   * Rebuild the desktop tree
   */
  void xworkspaces_module::rebuild_desktops() {
    m_viewports.clear();

    auto bounds = [&] {
      if (m_monitorsupport) {
        return ewmh_util::get_desktop_viewports();
      } else {
        vector<position> b;
        std::fill_n(std::back_inserter(b), m_desktop_names.size(), position{m_bar.monitor->x, m_bar.monitor->y});
        return b;
      }
    }();

    bounds.erase(std::unique(bounds.begin(), bounds.end(), [](auto& a, auto& b) { return a == b; }), bounds.end());

    unsigned int step;
    if (bounds.size() > 0) {
      step = m_desktop_names.size() / bounds.size();
    } else {
      step = 1;
    }
    unsigned int offset = 0;
    for (unsigned int i = 0; i < bounds.size(); i++) {
      if (!m_pinworkspaces || m_bar.monitor->match(bounds[i])) {
        auto viewport = make_unique<struct viewport>();
        viewport->state = viewport_state::UNFOCUSED;
        viewport->pos = bounds[i];

        for (auto&& m : m_monitors) {
          if (m->match(viewport->pos)) {
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

        for (unsigned int index = offset; index < offset + step; index++) {
          viewport->desktops.emplace_back(make_unique<struct desktop>(index, offset, desktop_state::EMPTY, label_t{}));
        }

        m_viewports.emplace_back(move(viewport));
      }
      offset += step;
    }
  }

  /**
   * Update active state of current desktops
   */
  void xworkspaces_module::rebuild_desktop_states() {
    for (auto&& v : m_viewports) {
      for (auto&& d : v->desktops) {
        if (m_desktop_names[d->index] == m_current_desktop_name) {
          d->state = desktop_state::ACTIVE;
        } else {
          d->state = desktop_state::EMPTY;
        } 

        d->label = m_labels.at(d->state)->clone();
        d->label->reset_tokens();
        d->label->replace_token("%index%", to_string(d->index - d->offset + 1));
        d->label->replace_token("%name%", m_desktop_names[d->index]);
        d->label->replace_token("%icon%", m_icons->get(m_desktop_names[d->index], DEFAULT_ICON)->get());
      }
    }
  }

  vector<string> xworkspaces_module::get_desktop_names(){
    vector<string> names = ewmh_util::get_desktop_names();
    unsigned int desktops_number = ewmh_util::get_number_of_desktops();
    if(desktops_number == names.size()) {
      return names;
    }
    else if(desktops_number < names.size()) {
      names.erase(names.begin()+desktops_number, names.end());
      return names;
    }
    for (unsigned int i = names.size(); i < desktops_number + 1; i++) {
      names.insert(names.end(), to_string(i));
    }
    return names;
  }

  /**
   * Find window and set corresponding desktop to urgent
   */
  void xworkspaces_module::set_desktop_urgent(xcb_window_t window) {
    auto desk = ewmh_util::get_desktop_from_window(window);
    if(desk == m_current_desktop)
      // ignore if current desktop is urgent
      return;
    for (auto&& v : m_viewports) {
      for (auto&& d : v->desktops) {
        if (d->index == desk && d->state != desktop_state::URGENT) {
          d->state = desktop_state::URGENT;

          d->label = m_labels.at(d->state)->clone();
          d->label->reset_tokens();
          d->label->replace_token("%index%", to_string(d->index - d->offset + 1));
          d->label->replace_token("%name%", m_desktop_names[d->index]);
          d->label->replace_token("%icon%", m_icons->get(m_desktop_names[d->index], DEFAULT_ICON)->get());
          return;
        }
      }
    }

  }

  /**
   * Fetch and parse data
   */
  void xworkspaces_module::update() {}

  /**
   * Generate module output
   */
  string xworkspaces_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapped
    // with the cmd handlers
    string output;
    for (m_index = 0; m_index < m_viewports.size(); m_index++) {
      if (m_index > 0) {
        m_builder->space(m_formatter->get(DEFAULT_FORMAT)->spacing);
      }
      output += module::get_output();
    }

    if (m_scroll) {
      m_builder->cmd(mousebtn::SCROLL_DOWN, string{EVENT_PREFIX} + string{EVENT_SCROLL_DOWN});
      m_builder->cmd(mousebtn::SCROLL_UP, string{EVENT_PREFIX} + string{EVENT_SCROLL_UP});
    }

    m_builder->append(output);

    m_builder->cmd_close();
    m_builder->cmd_close();

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
            builder->cmd(mousebtn::LEFT, string{EVENT_PREFIX} + string{EVENT_CLICK} + to_string(desktop->index));
            builder->node(desktop->label);
            builder->cmd_close();
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

  /**
   * Handle user input event
   */
  bool xworkspaces_module::input(string&& cmd) {
    size_t len{strlen(EVENT_PREFIX)};
    if (cmd.compare(0, len, EVENT_PREFIX) != 0) {
      return false;
    }
    cmd.erase(0, len);

    vector<unsigned int> indexes;
    for (auto&& viewport : m_viewports) {
      for (auto&& desktop : viewport->desktops) {
        indexes.emplace_back(desktop->index);
      }
    }

    std::sort(indexes.begin(), indexes.end());

    unsigned int new_desktop{0};
    unsigned int current_desktop{ewmh_util::get_current_desktop()};

    if ((len = strlen(EVENT_CLICK)) && cmd.compare(0, len, EVENT_CLICK) == 0) {
      new_desktop = std::strtoul(cmd.substr(len).c_str(), nullptr, 10);
    } else if ((len = strlen(EVENT_SCROLL_UP)) && cmd.compare(0, len, EVENT_SCROLL_UP) == 0) {
      new_desktop = math_util::min<unsigned int>(indexes.back(), current_desktop + 1);
      new_desktop = new_desktop == current_desktop ? indexes.front() : new_desktop;
    } else if ((len = strlen(EVENT_SCROLL_DOWN)) && cmd.compare(0, len, EVENT_SCROLL_DOWN) == 0) {
      new_desktop = math_util::max<unsigned int>(indexes.front(), current_desktop - 1);
      new_desktop = new_desktop == current_desktop ? indexes.back() : new_desktop;
    }

    if (new_desktop != current_desktop) {
      m_log.info("%s: Requesting change to desktop #%u", name(), new_desktop);
      ewmh_util::change_current_desktop(new_desktop);
    } else {
      m_log.info("%s: Ignoring change to current desktop", name());
    }

    return true;
  }
}

POLYBAR_NS_END
