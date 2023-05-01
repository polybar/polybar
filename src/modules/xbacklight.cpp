#include "modules/xbacklight.hpp"

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "utils/math.hpp"
#include "x11/connection.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

namespace modules {
  template class module<xbacklight_module>;

  /**
   * Construct module
   */
  xbacklight_module::xbacklight_module(const bar_settings& bar, string name_, const config& config)
      : static_module<xbacklight_module>(bar, move(name_), config), m_connection(connection::make()) {
    m_router->register_action(EVENT_INC, [this]() { action_inc(); });
    m_router->register_action(EVENT_DEC, [this]() { action_dec(); });

    auto output = m_conf.get(name(), "output", m_bar.monitor->name);

    auto monitors = randr_util::get_monitors(m_connection, bar.monitor_strict, false);

    m_output = randr_util::match_monitor(monitors, output, bar.monitor_exact);

    // If we didn't get a match we stop the module
    if (!m_output) {
      throw module_error("No matching output found for \"" + output + "\", stopping module...");
    }

    // Get flag to check if we should add scroll handlers for changing value
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);

    // Query randr for the backlight max and min value
    try {
      auto& backlight = m_output->backlight;
      randr_util::get_backlight_range(m_connection, m_output, backlight);
      randr_util::get_backlight_value(m_connection, m_output, backlight);
    } catch (const exception& err) {
      m_log.err("%s: Could not get data (err: %s)", name(), err.what());
      throw module_error("Not supported for \"" + m_output->name + "\"");
    }

    // Create window that will proxy all RandR notify events
    // clang-format off
    m_proxy = winspec(m_connection)
      << cw_size(1, 1)
      << cw_pos(-1, -1)
      << cw_flush(true);
    // clang-format on

    // Connect with the event registry and make sure we get
    // notified when a RandR output property gets modified
    m_connection.select_input_checked(m_proxy, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR, TAG_RAMP});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%%");
    }
    if (m_formatter->has(TAG_BAR)) {
      m_progressbar = load_progressbar(m_bar, m_conf, name(), TAG_BAR);
    }
    if (m_formatter->has(TAG_RAMP)) {
      m_ramp = load_ramp(m_conf, name(), TAG_RAMP);
    }
  }

  /**
   * Handler for XCB_RANDR_NOTIFY events
   */
  void xbacklight_module::handle(const evt::randr_notify& evt) {
    if (evt->subCode != XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
      return;
    } else if (evt->u.op.status != XCB_PROPERTY_NEW_VALUE) {
      return;
    } else if (evt->u.op.window != m_proxy) {
      return;
    } else if (evt->u.op.output != m_output->output) {
      return;
    } else if (evt->u.op.atom != m_output->backlight.atom) {
      return;
    } else {
      update();
    }
  }

  /**
   * Query the RandR extension for the new values
   */
  void xbacklight_module::update() {
    auto& bl = m_output->backlight;
    randr_util::get_backlight_value(m_connection, m_output, bl);
    m_percentage = math_util::nearest_5(math_util::percentage<double>(bl.val, bl.min, bl.max));

    // Update label tokens
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(m_percentage));
    }

    // Emit a broadcast notification so that
    // the new data will be drawn to the bar
    broadcast();
  }

  /**
   * Generate the module output
   */
  string xbacklight_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapped
    // with the cmd handlers
    string output{module::get_output()};

    if (m_scroll) {
      m_builder->action(mousebtn::SCROLL_UP, *this, EVENT_INC, "");
      m_builder->action(mousebtn::SCROLL_DOWN, *this, EVENT_DEC, "");
    }

    m_builder->node(output);

    m_builder->action_close();
    m_builder->action_close();

    return m_builder->flush();
  }

  /**
   * Output content as defined in the config
   */
  bool xbacklight_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR) {
      builder->node(m_progressbar->output(m_percentage));
    } else if (tag == TAG_RAMP) {
      builder->node(m_ramp->get_by_percentage(m_percentage));
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }
    return true;
  }

  void xbacklight_module::action_inc() {
    change_value(5);
  }

  void xbacklight_module::action_dec() {
    change_value(-5);
  }

  void xbacklight_module::change_value(int value_mod) {
    m_log.info("%s: Changing value by %i%", name(), value_mod);
    int rounded = math_util::cap<double>(m_percentage + value_mod, 0.0, 100.0) + 0.5;

    const int values[1]{math_util::percentage_to_value<int>(rounded, m_output->backlight.max)};

    m_connection.change_output_property_checked(
        m_output->output, m_output->backlight.atom, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, values);
  }
} // namespace modules

POLYBAR_NS_END
