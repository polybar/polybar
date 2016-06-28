#include "utils/xcb.hpp"
#include "utils/memory.hpp"
#include "services/logger.hpp"

namespace xcb
{
  namespace connection
  {
    bool check(xcb_connection_t *connection)
    {
      int err;

      if ((err = xcb_connection_has_error(connection)) == 0)
        return true;

      log_error("xcb: The server closed the connection");

      switch (err) {
        case XCB_CONN_ERROR:
          log_error("- socket, pipe or stream error");
          break;
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
          log_error("- unsupported extension");
          break;
        case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
          log_error("- not enough memory");
          break;
        case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
          log_error("- request length exceeded");
          break;
        case XCB_CONN_CLOSED_PARSE_ERR:
          log_error("- can't parse display string");
          break;
        case XCB_CONN_CLOSED_INVALID_SCREEN:
          log_error("- invalid screen");
          break;
        case XCB_CONN_CLOSED_FDPASSING_FAILED:
          log_error("- failed to pass FD");
          break;
        default:
          log_error("- unknown error");
          break;
      }

      return false;
    }
  }

  namespace monitor
  {
    std::shared_ptr<monitor_t> make_object() {
      return memory::make_malloc_ptr<monitor_t>();
    }

    std::shared_ptr<monitor_t> make_object(char *name, size_t name_len, int idx, xcb_rectangle_t rect)
    {
      auto mon = make_object();

      mon->bounds = rect;
      mon->index = idx;

      size_t name_size = name_len + 1;
      if (sizeof(mon->name) < name_size)
        name_size = sizeof(mon->name);

      std::snprintf(mon->name, name_size, "%s", name);

      return mon;
    }

    std::vector<std::shared_ptr<monitor_t>> get_list(xcb_connection_t *connection, xcb_window_t root)
    {
      std::vector<std::shared_ptr<monitor_t>> monitors;

      xcb_randr_get_screen_resources_reply_t *sres =
        xcb_randr_get_screen_resources_reply(connection,
          xcb_randr_get_screen_resources(connection, root), nullptr);

      if (sres == nullptr)
        return monitors;

      int len = xcb_randr_get_screen_resources_outputs_length(sres);
      xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(sres);

      for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_cookie_t cookie =
          xcb_randr_get_output_info(connection, outputs[i], XCB_CURRENT_TIME);
        xcb_randr_get_output_info_reply_t *info =
          xcb_randr_get_output_info_reply(connection, cookie, nullptr);
        xcb_randr_get_output_info(connection, outputs[i], XCB_CURRENT_TIME);

        if (info == nullptr)
          continue;

        if (info->crtc == XCB_NONE) {
          free(info);
          continue;
        }

        xcb_randr_get_crtc_info_reply_t *cir =
          xcb_randr_get_crtc_info_reply(connection,
            xcb_randr_get_crtc_info(connection, info->crtc, XCB_CURRENT_TIME), nullptr);

        if (cir == nullptr) {
          free(info);
          continue;
        }

        char *monitor_name = (char *) xcb_randr_get_output_info_name(info);
        monitors.emplace_back(make_object(monitor_name, info->name_len, i,
            {cir->x, cir->y, cir->width, cir->height}));

        free(cir);
      }

      std::sort(monitors.begin(), monitors.end(), [](std::shared_ptr<monitor_t> m1, std::shared_ptr<monitor_t> m2) -> bool
      {
        if (m1->bounds.x < m2->bounds.x || m1->bounds.y + m1->bounds.height <= m2->bounds.y)
          return 1;
        if (m1->bounds.x > m2->bounds.x || m1->bounds.y + m1->bounds.height > m2->bounds.y)
          return -1;
        return 0;
      });

      return monitors;
    }
  }
}
