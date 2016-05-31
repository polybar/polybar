#pragma once

#include <string>
#include <memory>
#include <net/if.h>
#include <iwlib.h>
#include <limits.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "exception.hpp"
#include "services/command.hpp"

namespace net
{
  bool is_wireless_interface(const std::string& ifname);

  // Network

  class NetworkException : public Exception
  {
    public:
      explicit NetworkException(const std::string& msg)
        : Exception("[Network] "+ msg){}
  };

  class Network
  {
    protected:
      std::unique_ptr<Command> ping;
      std::string interface;
      struct ifreq data;
      int fd;

      bool test_interface();
      bool test_connection();

    public:
      explicit Network(const std::string& interface);
      ~Network();

      virtual bool connected();
      virtual bool test();

      std::string get_ip();
  };

  // WiredNetwork

  class WiredNetworkException : public NetworkException {
    using NetworkException::NetworkException;
  };

  class WiredNetwork : public Network
  {
    int linkspeed = 0;

    public:
      explicit WiredNetwork(const std::string& interface);

      std::string get_link_speed();
  };

  // WirelessNetwork

  class WirelessNetworkException : public NetworkException {
    using NetworkException::NetworkException;
  };

  class WirelessNetwork : public Network
  {
    struct iwreq iw;

    public:
      explicit WirelessNetwork(const std::string& interface);

      std::string get_essid();
      float get_signal_dbm();
      float get_signal_quality();
  };


}
