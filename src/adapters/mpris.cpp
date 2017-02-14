#include <dbus/dbus.h>
#include <adapters/mpris.hpp>
#include <common.hpp>

POLYBAR_NS

namespace mpris {

  void mprisconnection::pause_play() {
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
  }

  std::string mprisconnection::get_playback_status() {
    DBusError err;
    DBusConnection* conn;
    dbus_error_init(&err);
    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter args;


    auto full_player_name = "org.mpris.MediaPlayer2." + player;

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    msg = dbus_message_new_method_call(full_player_name.data(), "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get");

    if (msg == nullptr) {
      m_log.err("Empty message");
      return "";
    }

    dbus_message_iter_init_append(msg, &args);

    const char* interface_name = "org.mpris.MediaPlayer2.Player";
    const char* property_name = "PlaybackStatus";

    if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface_name)) {
        m_log.err("Out of memory");
        return "";
    }

    if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &property_name)) {
        m_log.err("Out of memory");
        return "";
    }

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
      m_log.err("Out of memory");
      return "";
    }

    if (pending == nullptr) {
      m_log.err("Pending call null");
      return "";
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);

    if (msg == NULL) {
        m_log.err("Reply Null");
        return "";
    }

    dbus_pending_call_unref(pending);

    if (!dbus_message_iter_init(msg, &args)) {
        m_log.err("No arguments");
        return "";
    }

    const char* playback_status;

    int current_type = dbus_message_iter_get_arg_type(&args);

    m_log.err("Arg type: %d", current_type);

    DBusMessageIter iter;
    dbus_message_iter_recurse(&args, &iter);
    dbus_message_iter_get_basic(&iter, &playback_status);

    m_log.err("PLAYBACK: %s", playback_status);

    return std::string(playback_status);
  }
}
POLYBAR_NS_END
