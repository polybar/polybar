#include <mpris-generated.h>
#include <string.h>
#include <adapters/mpris.hpp>
#include <common.hpp>
#include <iostream>

POLYBAR_NS

namespace mpris {

  // https://developer.gnome.org/glib/stable/glib-GVariant.html#g-variant-iter-loop

  PolybarOrgMprisMediaPlayer2Player* mprisconnection::get_object() {
    GError* error = nullptr;

    auto player_object = "org.mpris.MediaPlayer2." + player;

    auto object = polybar_org_mpris_media_player2_player_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE, player_object.data(), "/org/mpris/MediaPlayer2", NULL, &error);

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

  mprissong mprisconnection::get_song() {
    auto object = get_object();

    if (object == nullptr) {
      return mprissong();
    }

    GVariantIter iter;
    GVariant* value;
    gchar* key;

    string title;
    string album;
    string artist;
    unsigned duration = -1;

    auto variant = polybar_org_mpris_media_player2_player_get_metadata(object);

    if (variant == nullptr) {
        return mprissong();
    }

    auto size = g_variant_iter_init(&iter, variant);

    if (size == 0) {
        return mprissong();
    }

    while (g_variant_iter_loop(&iter, "{sv}", &key, &value)) {
      if (strcmp(key, "xesam:album") == 0) {
        album = g_variant_get_string(value, nullptr);
      } else if (strcmp(key, "xesam:title") == 0) {
        title = g_variant_get_string(value, nullptr);
      }
    }

    return mprissong(title, album, artist, duration);
  }
}

POLYBAR_NS_END
