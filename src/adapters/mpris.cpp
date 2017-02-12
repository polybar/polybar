#include <adapters/mpris.hpp>
#include <dbus/dbus.h>

POLYBAR_NS

namespace mpris {

mprissong mprisconnection::get_current_song() {

  DBusError err;
  DBusConnection* conn;
  int ret;
  // initialise the errors
  dbus_error_init(&err);
  DBusMessage* msg;
  DBusMessageIter args;
  uint32_t serial = 0;

  dbus_bus_get(DBUS_BUS_SESSION, &err);

  msg = dbus_message_new_method_call("org.mpris.MediaPlayer2.spotify",
                               "/org/mpris/MediaPlayer2",
                               "org.mpris.MediaPlayer2.Player",
                               "PlayPause");

  if (NULL == msg)
  {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

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
