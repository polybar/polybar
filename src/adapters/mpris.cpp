#include <mpris-generated.h>
#include <adapters/mpris.hpp>
#include <common.hpp>
#include <iostream>

POLYBAR_NS

namespace mpris {

  // https://developer.gnome.org/glib/stable/glib-GVariant.html#g-variant-iter-loop

  PolybarOrgMprisMediaPlayer2Player* mprisconnection::get_object() {
    GError* error = nullptr;

    auto object = polybar_org_mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE, "org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
      return nullptr;
    }

    return object;
  }

  void mprisconnection::pause_play() {
    GError* error = nullptr;

    auto object = get_object();

    if (object == nullptr) {
      return;
    }

    polybar_org_mpris_media_player2_player_call_play_pause_sync(object, NULL, &error);

    if (error != nullptr) {
      m_log.err("Empty session bus");
      m_log.err(std::string(error->message));
    }
  }
}

POLYBAR_NS_END
