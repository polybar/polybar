#include "adapters/net.hpp"

#include <iomanip>

#include <limits.h>
#include <linux/ethtool.h>
#include <linux/if_link.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef inline
#undef inline
#endif

#include "common.hpp"
#include "config.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace net {
  /**
   * Test if interface with given name is a wireless device
   */
  bool is_wireless_interface(string ifname) {
    return file_util::exists("/sys/class/net/" + ifname + "/wireless");
  }

  // class : network {{{

  /**
   * Construct network interface
   */
  network::network(string interface) : m_interface(interface) {
    if (if_nametoindex(m_interface.c_str()) == 0)
      throw network_error("Invalid network interface \"" + m_interface + "\"");
    if ((m_socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      throw network_error("Failed to open socket");
  }

  /**
   * Destruct network interface
   */
  network::~network() {
    if (m_socketfd != -1)
      close(m_socketfd);
  }

  /**
   * Query device driver for information
   */
  bool network::query() {
    struct ifaddrs* ifaddr;

    if (getifaddrs(&ifaddr) == -1)
      return false;

    for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if (m_interface.compare(0, m_interface.length(), ifa->ifa_name) != 0)
        continue;

      switch (ifa->ifa_addr->sa_family) {
        case AF_INET:
          char ip_buffer[NI_MAXHOST];
          getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), ip_buffer, NI_MAXHOST, nullptr, 0,
              NI_NUMERICHOST);
          m_status.ip = string{ip_buffer};
          break;

        case AF_PACKET:
          if (ifa->ifa_data == nullptr)
            continue;
          struct rtnl_link_stats* link_state =
              reinterpret_cast<decltype(link_state)>(ifa->ifa_data);
          m_status.previous = m_status.current;
          m_status.current.transmitted = link_state->tx_bytes;
          m_status.current.received = link_state->rx_bytes;
          m_status.current.time = chrono::system_clock::now();
          break;
      }
    }

    freeifaddrs(ifaddr);

    return true;
  }

  /**
   * Run ping command to test internet connectivity
   */
  bool network::ping() const {
    try {
      auto exec = "ping -c 2 -W 2 -I " + m_interface + " " + string(CONNECTION_TEST_IP);
      auto ping = command_util::make_command(exec);
      return ping && ping->exec(true) == EXIT_SUCCESS;
    } catch (const std::exception& err) {
      return false;
    }
  }

  /**
   * Get interface ip address
   */
  string network::ip() const {
    return m_status.ip;
  }

  /**
   * Get download speed rate
   */
  string network::downspeed(int minwidth) const {
    float bytes_diff = m_status.current.received - m_status.previous.received;
    return format_speedrate(bytes_diff, minwidth);
  }

  /**
   * Get upload speed rate
   */
  string network::upspeed(int minwidth) const {
    float bytes_diff = m_status.current.transmitted - m_status.previous.transmitted;
    return format_speedrate(bytes_diff, minwidth);
  }

  /**
   * Test if the network interface is in a valid state
   */
  bool network::test_interface() const {
    auto operstate = file_util::get_contents("/sys/class/net/" + m_interface + "/operstate");
    return operstate.compare(0, 2, "up") == 0;
  }

  /**
   * Format up- and download speed
   */
  string network::format_speedrate(float bytes_diff, int minwidth) const {
    const auto duration = m_status.current.time - m_status.previous.time;
    float time_diff = chrono::duration_cast<chrono::seconds>(duration).count();
    float speedrate = bytes_diff / (time_diff ? time_diff : 1);

    vector<string> suffixes{"GB", "MB"};
    string suffix{"KB"};

    while ((speedrate /= 1000) > 999) {
      suffix = suffixes.back();
      suffixes.pop_back();
    }

    return string_util::from_stream(stringstream() << std::setw(minwidth) << std::setfill(' ')
                                                   << std::setprecision(0) << std::fixed
                                                   << speedrate << " " << suffix << "/s");
  }

  // }}}
  // class : wired_network {{{

  /**
   * Query device driver for information
   */
  bool wired_network::query() {
    if (!network::query())
      return false;

    struct ethtool_cmd ethernet_data;
    ethernet_data.cmd = ETHTOOL_GSET;

    struct ifreq request;
    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);
    request.ifr_data = reinterpret_cast<caddr_t>(&ethernet_data);

    if (ioctl(m_socketfd, SIOCETHTOOL, &request) == -1)
      return false;

    m_linkspeed = ethernet_data.speed;

    return true;
  }

  /**
   * Check current connection state
   */
  bool wired_network::connected() const {
    if (!network::test_interface())
      return false;

    struct ifreq request;
    struct ethtool_value ethernet_data;

    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);
    ethernet_data.cmd = ETHTOOL_GLINK;
    request.ifr_data = reinterpret_cast<caddr_t>(&ethernet_data);

    if (ioctl(m_socketfd, SIOCETHTOOL, &request) != -1)
      return ethernet_data.data != 0;

    return false;
  }

  /**
   *
   * about the current connection
   */
  string wired_network::linkspeed() const {
    return (m_linkspeed == 0 ? "???" : to_string(m_linkspeed)) + " Mbit/s";
  }

  // }}}
  // class : wireless_network {{{

  /**
   * Query the wireless device for information
   * about the current connection
   */
  bool wireless_network::query() {
    if (!network::query())
      return false;

    auto socket_fd = iw_sockets_open();

    if (socket_fd == -1)
      return false;

    struct iwreq req;

    if (iw_get_ext(socket_fd, m_interface.c_str(), SIOCGIWMODE, &req) == -1)
      return false;

    // Ignore interfaces in ad-hoc mode
    if (req.u.mode == IW_MODE_ADHOC)
      return false;

    query_essid(socket_fd);
    query_quality(socket_fd);

    iw_sockets_close(socket_fd);

    return true;
  }

  /**
   * Check current connection state
   */
  bool wireless_network::connected() const {
    if (!network::test_interface())
      return false;

    struct ifreq request;
    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);

    if ((ioctl(m_socketfd, SIOCGIFFLAGS, &request)) == -1)
      return false;
    if (m_essid.empty())
      return false;

    return true;
  }

  /**
   * ESSID reported by last query
   */
  string wireless_network::essid() const {
    return m_essid;
  }

  /**
   * Signal strength percentage reported by last query
   */
  int wireless_network::signal() const {
    return m_signalstrength.percentage();
  }

  /**
   * Link quality percentage reported by last query
   */
  int wireless_network::quality() const {
    return m_linkquality.percentage();
  }

  /**
   * Query for ESSID
   */
  void wireless_network::query_essid(const int& socket_fd) {
    char essid[IW_ESSID_MAX_SIZE + 1];

    struct iwreq req;
    req.u.essid.pointer = &essid;
    req.u.essid.length = sizeof(essid);
    req.u.essid.flags = 0;

    if (iw_get_ext(socket_fd, m_interface.c_str(), SIOCGIWESSID, &req) != -1) {
      m_essid = string{essid};
    } else {
      m_essid.clear();
    }
  }

  /**
   * Query for device driver quality values
   */
  void wireless_network::query_quality(const int& socket_fd) {
    iwrange range;
    iwstats stats;

    // Fill range
    if (iw_get_range_info(socket_fd, m_interface.c_str(), &range) == -1)
      return;
    // Fill stats
    if (iw_get_stats(socket_fd, m_interface.c_str(), &stats, &range, 1) == -1)
      return;

    // Check if the driver supplies the quality value
    if (stats.qual.updated & IW_QUAL_QUAL_INVALID)
      return;
    // Check if the driver supplies the quality level value
    if (stats.qual.updated & IW_QUAL_LEVEL_INVALID)
      return;

    // Check if the link quality has been uodated
    if (stats.qual.updated & IW_QUAL_QUAL_UPDATED) {
      m_linkquality.val = stats.qual.qual;
      m_linkquality.max = range.max_qual.qual;
    }

    // Check if the signal strength has been uodated
    if (stats.qual.updated & IW_QUAL_LEVEL_UPDATED) {
      m_signalstrength.val = stats.qual.level;
      m_signalstrength.max = range.max_qual.level;

      // Check if the values are defined in dBm
      if (stats.qual.level > range.max_qual.level) {
        m_signalstrength.val -= 0x100;
        m_signalstrength.max -= 0x100;
      }
    }
  }

  // }}}
}

LEMONBUDDY_NS_END
