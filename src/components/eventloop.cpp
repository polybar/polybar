#include "components/eventloop.hpp"

POLYBAR_NS

eventloop::eventloop() {
  m_loop = std::make_unique<uv_loop_t>();
  uv_loop_init(m_loop.get());
  // TODO handle return value

  m_loop->data = this;
}

eventloop::~eventloop() {
  if (m_loop) {
    uv_loop_close(m_loop.get());
    // TODO handle return value
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
