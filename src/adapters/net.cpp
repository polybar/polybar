#include "adapters/net.hpp"

#include <iomanip>

#include <arpa/inet.h>
#include <linux/ethtool.h>
#include <linux/if_link.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "common.hpp"
#include "settings.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace net {
  /**
   * Test if interface with given name is a wireless device
   */
  bool is_wireless_interface(const string& ifname) {
    return file_util::exists("/sys/class/net/" + ifname + "/wireless");
  }

  static const string NO_IP = string("N/A");

  // class : network {{{

  /**
   * Construct network interface
   */
  network::network(string interface) : m_log(logger::make()), m_interface(move(interface)) {
    if (if_nametoindex(m_interface.c_str()) == 0) {
      throw network_error("Invalid network interface \"" + m_interface + "\"");
    }

    m_socketfd = file_util::make_file_descriptor(socket(AF_INET, SOCK_DGRAM, 0));
    if (!*m_socketfd) {
      throw network_error("Failed to open socket");
    }

    check_tuntap_or_bridge();
  }

  /**
   * Query device driver for information
   */
  bool network::query(bool accumulate) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1 || ifaddr == nullptr) {
      return false;
    }

    m_status.previous = m_status.current;
    m_status.current.transmitted = 0;
    m_status.current.received = 0;
    m_status.current.time = std::chrono::system_clock::now();
    m_status.ip = NO_IP;
    m_status.ip6 = NO_IP;

    for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == nullptr) {
        continue;
      }

      if (m_interface.compare(0, m_interface.length(), ifa->ifa_name) != 0) {
        if (!accumulate || (ifa->ifa_data == nullptr && ifa->ifa_addr->sa_family != AF_PACKET)) {
          continue;
        }
      }

      struct sockaddr_in6* sa6;

      switch (ifa->ifa_addr->sa_family) {
        case AF_INET:
          char ip_buffer[NI_MAXHOST];
          getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), ip_buffer, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
          m_status.ip = string{ip_buffer};
          break;

        case AF_INET6:
          char ip6_buffer[INET6_ADDRSTRLEN];
          sa6 = reinterpret_cast<decltype(sa6)>(ifa->ifa_addr);
          if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr)) {
              continue;
          }
          if (IN6_IS_ADDR_SITELOCAL(&sa6->sin6_addr)) {
              continue;
          }
          if ((((unsigned char*)sa6->sin6_addr.s6_addr)[0] & 0xFE) == 0xFC) {
              /* Skip Unique Local Addresses (fc00::/7) */
              continue;
          }
          if (inet_ntop(AF_INET6, &sa6->sin6_addr, ip6_buffer, INET6_ADDRSTRLEN) == 0) {
              m_log.warn("inet_ntop() " + string(strerror(errno)));
              continue;
          }
          m_status.ip6 = string{ip6_buffer};
          break;

        case AF_PACKET:
          if (ifa->ifa_data == nullptr) {
            continue;
          }
          struct rtnl_link_stats* link_state = reinterpret_cast<decltype(link_state)>(ifa->ifa_data);
          if (link_state == nullptr) {
            continue;
          }
          m_status.current.transmitted += link_state->tx_bytes;
          m_status.current.received += link_state->rx_bytes;
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
   * Get interface ipv4 address
   */
  string network::ip() const {
    return m_status.ip;
  }

  /**
   * Get interface ipv6 address
   */
  string network::ip6() const {
    return m_status.ip6;
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
   * Set if unknown counts as up
   */
  void network::set_unknown_up(bool unknown) {
    m_unknown_up = unknown;
  }

  /**
   * Query driver info to check if the
   * interface is a TUN/TAP device or BRIDGE
   */
  void network::check_tuntap_or_bridge() {
    struct ethtool_drvinfo driver {};
    struct ifreq request {};

    driver.cmd = ETHTOOL_GDRVINFO;

    memset(&request, 0, sizeof(request));

    /*
     * Only copy array size minus one bytes over to ensure there is a
     * terminating NUL byte (which is guaranteed by memset)
     */
    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);

    request.ifr_data = reinterpret_cast<char*>(&driver);

    if (ioctl(*m_socketfd, SIOCETHTOOL, &request) == -1) {
      return;
    }

    // Check if it's a TUN/TAP device
    if (strncmp(driver.bus_info, "tun", 3) == 0) {
      m_tuntap = true;
    } else if (strncmp(driver.bus_info, "tap", 3) == 0) {
      m_tuntap = true;
    } else {
      m_tuntap = false;
    }

    if (strncmp(driver.driver, "bridge", 6) == 0) {
      m_bridge = true;
    }

  }

  /**
   * Test if the network interface is in a valid state
   */
  bool network::test_interface() const {
    auto operstate = file_util::contents("/sys/class/net/" + m_interface + "/operstate");
    bool up = operstate.compare(0, 2, "up") == 0;
    return m_unknown_up ? (up || operstate.compare(0, 7, "unknown") == 0) : up;
  }

  /**
   * Format up- and download speed
   */
  string network::format_speedrate(float bytes_diff, int minwidth) const {
    const auto duration = m_status.current.time - m_status.previous.time;
    float time_diff = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    float speedrate = bytes_diff / (time_diff ? time_diff : 1);

    vector<string> suffixes{"GB", "MB"};
    string suffix{"KB"};

    while ((speedrate /= 1000) > 999) {
      suffix = suffixes.back();
      suffixes.pop_back();
    }

    return sstream() << std::setw(minwidth) << std::setfill(' ') << std::setprecision(0) << std::fixed << speedrate
                     << " " << suffix << "/s";
  }

  // }}}
  // class : wired_network {{{

  /**
   * Query device driver for information
   */
  bool wired_network::query(bool accumulate) {
    if (m_tuntap) {
      return true;
    } else if (!network::query(accumulate)) {
      return false;
    }

    if(m_bridge) {
      /* If bridge network then link speed cannot be computed
       * TODO: Identify the physical network in bridge and compute the link speed
       */
      return true;
    }

    struct ifreq request {};
    struct ethtool_cmd data {};

    memset(&request, 0, sizeof(request));
    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);
    data.cmd = ETHTOOL_GSET;
    request.ifr_data = reinterpret_cast<char*>(&data);

    if (ioctl(*m_socketfd, SIOCETHTOOL, &request) == -1) {
      return false;
    }

    m_linkspeed = data.speed;

    return true;
  }

  /**
   * Check current connection state
   */
  bool wired_network::connected() const {
    if (!m_tuntap && !network::test_interface()) {
      return false;
    }

    struct ethtool_value data {};
    struct ifreq request {};

    memset(&request, 0, sizeof(request));
    strncpy(request.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);
    data.cmd = ETHTOOL_GLINK;
    request.ifr_data = reinterpret_cast<char*>(&data);

    if (ioctl(*m_socketfd, SIOCETHTOOL, &request) == -1) {
      return false;
    }

    return data.data != 0;
  }

  /**
   *
   * about the current connection
   */
  string wired_network::linkspeed() const {
    return (m_linkspeed == 0 ? "???" : to_string(m_linkspeed)) + " Mbit/s";
  }

  // }}}

}  // namespace net

POLYBAR_NS_END
