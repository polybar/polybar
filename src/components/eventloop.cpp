#include "components/eventloop.hpp"

POLYBAR_NS

eventloop::eventloop() {
  m_loop = std::make_unique<uv_loop_t>();
  uv_loop_init(m_loop.get());
  // TODO handle return value

  m_loop->data = this;
}

static void close_walk_cb(uv_handle_t* handle, void*) {
  if (!uv_is_closing(handle)) {
    uv_close(handle, nullptr);
  }
}

/**
 * Completely closes everything in the loop.
 *
 * After this function returns, uv_loop_close can be called.
 */
static void close_loop(uv_loop_t* loop) {
  uv_walk(loop, close_walk_cb, nullptr);
  uv_run(loop, UV_RUN_DEFAULT);
  // TODO handle return value
}

eventloop::~eventloop() {
  if (m_loop) {
    close_loop(m_loop.get());
    uv_loop_close(m_loop.get());
    // TODO handle return value

    m_loop.reset();
  }
}

void eventloop::run() {
  uv_run(m_loop.get(), UV_RUN_DEFAULT);
  // TODO handle return value
}

void eventloop::stop() {
  uv_stop(m_loop.get());
  // TODO handle return value
}

POLYBAR_NS_END
