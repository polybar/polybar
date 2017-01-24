#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "x11/extensions/randr.hpp"
#include "x11/types.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class config;
class logger;
class connection;
class signal_emitter;

class screen : public xpp::event::sink<evt::randr_screen_change_notify> {
 public:
  using make_type = unique_ptr<screen>;
  static make_type make();

  explicit screen(connection& conn, signal_emitter& emitter, const logger& logger, const config& conf);
  ~screen();

  struct size size() const {
    return m_size;
  }

  xcb_window_t root() const {
    return m_root;
  }

 protected:
  void handle(const evt::randr_screen_change_notify& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;

  xcb_window_t m_root;
  xcb_window_t m_proxy{XCB_NONE};

  vector<monitor_t> m_monitors;
  struct size m_size {0U, 0U};
  bool m_sigraised{false};
};

POLYBAR_NS_END
