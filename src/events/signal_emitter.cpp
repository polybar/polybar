#include "events/signal_emitter.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

signal_receivers_t g_signal_receivers;

/**
 * Create instance
 */
signal_emitter::make_type signal_emitter::make() {
  return static_cast<signal_emitter&>(*factory_util::singleton<signal_emitter>());
}

POLYBAR_NS_END
