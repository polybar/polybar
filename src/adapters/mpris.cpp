#include <mpris-generated.h>
#include <adapters/mpris.hpp>
#include <common.hpp>

POLYBAR_NS

namespace mpris {

  void mprisconnection::pause_play() {
    GError* error = nullptr;
    //auto dbus_connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    auto object = polybar_org_mpris_media_player2_player_proxy_new_for_bus_sync(
            G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_NONE,
            "org.mpris.MediaPlayer2.spotify",
            "/org/mpris/MediaPlayer2",
            NULL,
            &error);

    if (error != nullptr) {
        m_log.err("Empty session bus");
        m_log.err(std::string(error->message));
        return;
    }

    polybar_org_mpris_media_player2_player_call_play_pause_sync(object, NULL, &error);

    if (error != nullptr) {
        m_log.err("Empty session bus");
        m_log.err(std::string(error->message));
        return;
    }

   // g_object_unref(proxy);

    /*
    auto proxy = polybar_org_mpris_media_player2_player_proxy_new_sync(dbus_connection, G_DBUS_PROXY_FLAGS_NONE,
            "org.mpris.MediaPlayer2.spotify", "org.mpris.MediaPlayer2.spotify", NULL, &error);

    if (error != nullptr) {
        m_log.err(std::string(error->message));
        return;
    }

    polybar_org_mpris_media_player2_player_call_play_pause_sync(proxy, NULL, &error);


    if (error != nullptr) {
        m_log.err(std::string(error->message));
        return;
    }
    */
/*
    auto proxy = polybar_org_mpris_media_player2_player_proxy_new_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, "", "spotify", NULL, &error);

    polybar_org_mpris_media_player2_player_call_play_pause_sync(proxy, NULL, &error);
*/
    /*
  DBusError err;
  DBusConnection* conn;
  dbus_error_init(&err);
  DBusMessage* msg;
  DBusPendingCall* pending;

  auto full_player_name = "org.mpris.MediaPlayer2." + player;

  conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

  msg = dbus_message_new_method_call(
      full_player_name.data(), "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "PlayPause");

  if (NULL == msg) {
    m_log.err("Message is null");
    return;
  }

  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {  // -1 is default timeout
    m_log.err("Out of memory");
    return;
  }

  if (NULL == pending) {
    m_log.err("Pending Call Null");
    return;
  }

  dbus_connection_flush(conn);
  dbus_message_unref(msg);
  */
  }

  /*

  std::string mprisconnection::get_playback_status() {
    return get("PlaybackStatus");
  }

  std::string mprisconnection::get(std::string property) {
    DBusMessage* msg;
    DBusMessageIter args1;

    auto full_player_name = "org.mpris.MediaPlayer2." + player;

    msg = dbus_message_new_method_call(
        full_player_name.data(), "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");

    if (msg == nullptr) {
      m_log.err("Empty message");
      return "";
    }

    dbus_message_iter_init_append(msg, &args1);

    const char* interface_name = "org.mpris.MediaPlayer2.Player";
    const char* property_name = property.data();

    if (!dbus_message_iter_append_basic(&args1, DBUS_TYPE_STRING, &interface_name)) {
      m_log.err("Out of memory");
      return "";
    }

    if (!dbus_message_iter_append_basic(&args1, DBUS_TYPE_STRING, &property_name)) {
      m_log.err("Out of memory");
      return "";
    }

    auto args = send_message(msg);

    if (args == nullptr) {
      return "";
    }

    const char* playback_status;

    // Variant -> String
    DBusMessageIter iter;
    dbus_message_iter_recurse(args, &iter);
    dbus_message_iter_get_basic(&iter, &playback_status);

    delete args;

    return std::string(playback_status);
  }

  DBusMessageIter* mprisconnection::send_message(DBusMessage* msg) {
    DBusError err;
    DBusConnection* conn;
    dbus_error_init(&err);
    DBusPendingCall* pending;
    DBusMessageIter* args = new DBusMessageIter();

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
      m_log.err("Out of memory");
      return nullptr;
    }

    if (pending == nullptr) {
      m_log.err("Pending call null");
      return nullptr;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);

    if (msg == NULL) {
      m_log.err("Reply Null");
      return nullptr;
    }

    dbus_pending_call_unref(pending);

    if (!dbus_message_iter_init(msg, args)) {
      m_log.err("No arguments");
      return nullptr;
    }

    return args;
  }
  */
}
POLYBAR_NS_END
