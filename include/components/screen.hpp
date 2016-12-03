#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "x11/events.hpp"
#include "x11/window.hpp"

POLYBAR_NS

// fwd
class config;
class logger;
class connection;

class screen : public xpp::event::sink<evt::randr_screen_change_notify> {
 public:
  explicit screen(connection& conn, const logger& logger, const config& conf);
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
  const logger& m_log;
  const config& m_conf;

  xcb_window_t m_root;
  xcb_window_t m_proxy{XCB_NONE};

  vector<monitor_t> m_monitors;
  struct size m_size{0U, 0U};
  bool m_sigraised{false};
};

di::injector<unique_ptr<screen>> configure_screen();

POLYBAR_NS_END
