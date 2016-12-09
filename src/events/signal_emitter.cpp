#include "events/signal_emitter.hpp"
#include "events/signal_receiver.hpp"

POLYBAR_NS

signal_receivers_t g_signal_receivers;

/**
 * Create instance
 */
signal_emitter& signal_emitter::make() {
  auto instance = factory_util::singleton<signal_emitter>();
  return static_cast<signal_emitter&>(*instance);
}

POLYBAR_NS_END
