#include <adapters/mpris.hpp>
#include <dbus/dbus.h>
#include <common.hpp>

POLYBAR_NS

namespace mpris {

  mprissong mprisconnection::get_current_song() {

  DBusError err;
  DBusConnection* conn;
  // initialise the errors
  dbus_error_init(&err);
  DBusMessage* msg;
  DBusPendingCall* pending;

  conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

  msg = dbus_message_new_method_call("org.mpris.MediaPlayer2.spotify",
                               "/org/mpris/MediaPlayer2",
                               "org.mpris.MediaPlayer2.Player",
                               "PlayPause");

  if (NULL == msg)
  {
    m_log.err("Message is null");
    return mprissong();
  }

   // send message and get a handle for a reply
   if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
       m_log.err("Out of memory");
       return mprissong();
   }
   if (NULL == pending) {
       m_log.err("Pending Call Null");
       return mprissong();
   }
   dbus_connection_flush(conn);

   return mprissong();


/*  // append arguments onto signal
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }

  // send the message and flush the connection
  if (!dbus_connection_send(conn, msg, &serial)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  dbus_co*/


}


}

POLYBAR_NS_END
