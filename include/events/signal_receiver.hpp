#pragma once

#include <map>
#include <unordered_map>

#include "common.hpp"

POLYBAR_NS

class signal_receiver_interface {
 public:
  using prio = unsigned int;
  using prio_map = std::multimap<prio, signal_receiver_interface*>;
  virtual ~signal_receiver_interface() {}
  virtual prio priority() const = 0;
  template <typename Signal>
  bool on(const Signal& signal);
};

template <typename Signal>
class signal_receiver_impl {
 public:
  virtual ~signal_receiver_impl() {}
  virtual bool on(const Signal&) = 0;
};

template <typename Signal>
bool signal_receiver_interface::on(const Signal& s) {
  auto event_sink = dynamic_cast<signal_receiver_impl<Signal>*>(this);

  if (event_sink != nullptr && event_sink->on(s)) {
    return true;
  } else {
    return false;
  }
}

template <uint8_t Priority, typename Signal, typename... Signals>
class signal_receiver : public signal_receiver_interface,
                        public signal_receiver_impl<Signal>,
                        public signal_receiver_impl<Signals>... {
 public:
  prio priority() const {
    return Priority;
  }
};

using signal_receivers_t = std::unordered_map<uint8_t, signal_receiver_interface::prio_map>;

POLYBAR_NS_END
