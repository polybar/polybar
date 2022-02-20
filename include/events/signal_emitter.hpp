#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "events/signal_receiver.hpp"

POLYBAR_NS

/**
 * @brief Holds all signal receivers attached to the emitter
 */
extern signal_receivers_t g_signal_receivers;

/**
 * Wrapper used to delegate emitted signals
 * to attached signal receivers
 */
class signal_emitter {
 public:
  using make_type = signal_emitter&;
  static make_type make();

  explicit signal_emitter() = default;
  virtual ~signal_emitter() {}

  template <typename Signal>
  bool emit(const Signal& sig) {
    try {
      if (g_signal_receivers.find(id<Signal>()) != g_signal_receivers.end()) {
        for (auto&& item : g_signal_receivers.at(id<Signal>())) {
          if (item.second->on(sig)) {
            return true;
          }
        }
      }
    } catch (const std::exception& e) {
      logger::make().err("Signal receiver raised an exception: %s", e.what());
    }

    return false;
  }

  template <typename Signal, typename Next, typename... Signals>
  bool emit(const Signal& sig) const {
    return emit<Signal>(sig) || emit<Next, Signals...>(sig);
  }

  template <int Priority, typename Signal, typename... Signals>
  void attach(signal_receiver<Priority, Signal, Signals...>* s) {
    attach<signal_receiver<Priority, Signal, Signals...>, Signal, Signals...>(s);
  }

  template <int Priority, typename Signal, typename... Signals>
  void detach(signal_receiver<Priority, Signal, Signals...>* s) {
    detach<signal_receiver<Priority, Signal, Signals...>, Signal, Signals...>(s);
  }

 protected:
  template <typename Signal>
  std::type_index id() const {
    return typeid(Signal);
  }

  template <typename Receiver, typename Signal>
  void attach(Receiver* s) {
    attach(s, id<Signal>());
  }

  template <typename Receiver, typename Signal, typename Next, typename... Signals>
  void attach(Receiver* s) {
    attach(s, id<Signal>());
    attach<Receiver, Next, Signals...>(s);
  }

  void attach(signal_receiver_interface* s, std::type_index id) {
    g_signal_receivers[id].emplace(s->priority(), s);
  }

  template <typename Receiver, typename Signal>
  void detach(Receiver* s) {
    detach(s, id<Signal>());
  }

  template <typename Receiver, typename Signal, typename Next, typename... Signals>
  void detach(Receiver* s) {
    detach(s, id<Signal>());
    detach<Receiver, Next, Signals...>(s);
  }

  void detach(signal_receiver_interface* d, std::type_index id) {
    try {
      auto& prio_map = g_signal_receivers.at(id);
      const auto& prio_sink_pair = prio_map.equal_range(d->priority());

      for (auto it = prio_sink_pair.first; it != prio_sink_pair.second;) {
        if (d == it->second) {
          it = prio_map.erase(it);
        } else {
          ++it;
        }
      }
    } catch (...) {
    }
  }
};

POLYBAR_NS_END
