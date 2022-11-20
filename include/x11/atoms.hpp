#pragma once

#include <xcb/xcb_atom.h>

#include <array>
#include <string>

struct cached_atom {
  const std::string name;
  xcb_atom_t& atom;
};

extern std::array<cached_atom, 38> ATOMS;

extern xcb_atom_t _NET_SUPPORTED;
extern xcb_atom_t _NET_CURRENT_DESKTOP;
extern xcb_atom_t _NET_ACTIVE_WINDOW;
extern xcb_atom_t _NET_WM_NAME;
extern xcb_atom_t _NET_WM_DESKTOP;
extern xcb_atom_t _NET_WM_VISIBLE_NAME;
extern xcb_atom_t _NET_WM_WINDOW_TYPE;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
extern xcb_atom_t _NET_WM_PID;
extern xcb_atom_t _NET_WM_STATE;
extern xcb_atom_t _NET_WM_STATE_STICKY;
extern xcb_atom_t _NET_WM_STATE_SKIP_TASKBAR;
extern xcb_atom_t _NET_WM_STATE_ABOVE;
extern xcb_atom_t _NET_WM_STATE_MAXIMIZED_VERT;
extern xcb_atom_t _NET_WM_STRUT;
extern xcb_atom_t _NET_WM_STRUT_PARTIAL;
extern xcb_atom_t WM_PROTOCOLS;
extern xcb_atom_t WM_DELETE_WINDOW;
extern xcb_atom_t _XEMBED;
extern xcb_atom_t _XEMBED_INFO;
extern xcb_atom_t MANAGER;
extern xcb_atom_t WM_STATE;
extern xcb_atom_t _NET_SYSTEM_TRAY_OPCODE;
extern xcb_atom_t _NET_SYSTEM_TRAY_ORIENTATION;
extern xcb_atom_t _NET_SYSTEM_TRAY_VISUAL;
extern xcb_atom_t _NET_SYSTEM_TRAY_COLORS;
extern xcb_atom_t WM_TAKE_FOCUS;
extern xcb_atom_t Backlight;
extern xcb_atom_t BACKLIGHT;
extern xcb_atom_t _XROOTPMAP_ID;
extern xcb_atom_t _XSETROOT_ID;
extern xcb_atom_t ESETROOT_PMAP_ID;
extern xcb_atom_t _COMPTON_SHADOW;
extern xcb_atom_t _NET_WM_WINDOW_OPACITY;
extern xcb_atom_t WM_HINTS;
extern xcb_atom_t WM_NAME;
extern xcb_atom_t WM_CLASS;
