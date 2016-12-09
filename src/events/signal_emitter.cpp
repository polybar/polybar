#include "events/signal_emitter.hpp"

POLYBAR_NS

signal_receivers_t g_signal_receivers;

/**
 * Create instance
 */
signal_emitter::make_type signal_emitter::make() {
  auto instance = factory_util::singleton<signal_emitter>();
  return static_cast<signal_emitter&>(*instance);
}

POLYBAR_NS_END
