#include "components/eventloop.hpp"

POLYBAR_NS

eventloop::eventloop() {
  m_loop = std::make_unique<uv_loop_t>();
  UV(uv_loop_init, m_loop.get());
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
  UV(uv_run, loop, UV_RUN_DEFAULT);
}

eventloop::~eventloop() {
  if (m_loop) {
    try {
      close_loop(m_loop.get());
      UV(uv_loop_close, m_loop.get());
    } catch (const std::exception& e) {
      // TODO log error
    }

    m_loop.reset();
  }
}

void eventloop::run() {
  UV(uv_run, m_loop.get(), UV_RUN_DEFAULT);
}

void eventloop::stop() {
  uv_stop(m_loop.get());
}

POLYBAR_NS_END
