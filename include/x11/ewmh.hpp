#pragma once

#include <xcb/xcb_ewmh.h>

#include "common.hpp"
#include "utils/memory.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

struct position;

namespace ewmh_util {
  class ewmh_connection : public non_copyable_mixin, public non_movable_mixin {
   public:
    ewmh_connection();
    ~ewmh_connection();

    xcb_ewmh_connection_t* operator->();
    operator xcb_ewmh_connection_t*();

   private:
    xcb_ewmh_connection_t c;
  };

  ewmh_connection& initialize();

  bool supports(xcb_atom_t atom, int screen = 0);

  string get_wm_name(xcb_window_t win);
  string get_visible_name(xcb_window_t win);
  string get_icon_name(xcb_window_t win);
  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply);

  vector<position> get_desktop_viewports(int screen = 0);
  vector<string> get_desktop_names(int screen = 0);
  unsigned int get_current_desktop(int screen = 0);
  unsigned int get_number_of_desktops(int screen = 0);
  xcb_window_t get_active_window(int screen = 0);

  void change_current_desktop(unsigned int desktop);
  unsigned int get_desktop_from_window(xcb_window_t window);

  void set_wm_window_type(xcb_window_t win, vector<xcb_atom_t> types);

  void set_wm_state(xcb_window_t win, vector<xcb_atom_t> states);
  vector<xcb_atom_t> get_wm_state(xcb_window_t win);

  void set_wm_pid(xcb_window_t win);
  void set_wm_pid(xcb_window_t win, unsigned int pid);

  void set_wm_desktop(xcb_window_t win, unsigned int desktop = -1u);
  void set_wm_window_opacity(xcb_window_t win, unsigned long int values);

  vector<xcb_window_t> get_client_list(int screen = 0);

  xcb_window_t get_supporting_wm_check(xcb_window_t win);
  xcb_window_t get_ewmh_meta_window(xcb_window_t root);
} // namespace ewmh_util

POLYBAR_NS_END
