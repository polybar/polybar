#include "x11/window.hpp"
#include "x11/xutils.hpp"

LEMONBUDDY_NS

window window::create_checked(
    int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t mask, const xcb_params_cw_t* params) {
  auto screen = connection().screen();
  auto visual = connection().visual_type(screen, 32).get();
  auto depth = (visual->visual_id == screen->root_visual) ? XCB_COPY_FROM_PARENT : 32;
  uint32_t value_list[16];
  xutils::pack_values(mask, params, value_list);
  connection().create_window_checked(depth, operator*(), screen->root, x, y, w, h, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, visual->visual_id, mask, value_list);
  return *this;
}

window window::create_checked(
    uint16_t w, uint16_t h, uint32_t mask, const xcb_params_cw_t* params) {
  return create_checked(0, 0, w, h, mask, params);
}

LEMONBUDDY_NS_END
