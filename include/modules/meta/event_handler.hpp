#pragma once

#include "common.hpp"
#include "utils/concurrency.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

class connection;

namespace modules {
  struct event_handler_interface {
    virtual ~event_handler_interface() {}
    virtual void connect(connection&) {}
    virtual void disconnect(connection&) {}
  };

  template <typename Event, typename... Events>
  class event_handler : public event_handler_interface, public xpp::event::sink<Event, Events...> {
   public:
    virtual ~event_handler() {}

    virtual void connect(connection& conn) override {
      conn.attach_sink(this, SINK_PRIORITY_MODULE);
    }

    virtual void disconnect(connection& conn) override {
      conn.detach_sink(this, SINK_PRIORITY_MODULE);
    }
  };
}

POLYBAR_NS_END
