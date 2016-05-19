#ifndef _INTERFACES_NET_HPP_
#define _INTERFACES_NET_HPP_

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
      NetworkException(const std::string& msg)
        : Exception("[Network] "+ msg){}
  };

  class Network
  {
    protected:
      std::unique_ptr<Command> ping;
      std::string interface;
      struct ifreq data;
      int fd;

      bool test_interface() throw(NetworkException);
      bool test_connection() throw(NetworkException);

    public:
      Network(const std::string& interface) throw(NetworkException);
      ~Network();

      virtual bool connected();
      virtual bool test();

      std::string get_ip() throw(NetworkException);
  };


  // WiredNetwork

  class WiredNetworkException : public NetworkException {
    using NetworkException::NetworkException;
  };

  class WiredNetwork : public Network
  {
    int linkspeed = 0;

    public:
    WiredNetwork(const std::string& interface);

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
      WirelessNetwork(const std::string& interface);

      std::string get_essid() throw(WirelessNetworkException);
      float get_signal_dbm() throw(WirelessNetworkException);
      float get_signal_quality();
  };


}

#endif
