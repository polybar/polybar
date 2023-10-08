#pragma once

#include "common.hpp"
#include "utils/restack.hpp"
#include "utils/socket.hpp"

POLYBAR_NS

class connection;

namespace bspwm_util {
using connection_t = unique_ptr<socket_util::unix_connection>;

struct payload {
  char data[BUFSIZ]{'\0'};
  size_t len = 0;
};

using payload_t = unique_ptr<payload>;

restack_util::params get_restack_params(connection& conn);

string get_socket_path();

payload_t make_payload(const string& cmd);
connection_t make_connection();
connection_t make_subscriber();
} // namespace bspwm_util

POLYBAR_NS_END
