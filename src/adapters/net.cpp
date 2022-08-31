#include "adapters/net.hpp"

#include <arpa/inet.h>
#include <dirent.h>
#include <linux/ethtool.h>
#include <linux/if_link.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <iomanip>

#include "common.hpp"
#include "settings.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace net {
  enum class NetType {
    WIRELESS,
    ETHERNET,
    OTHER,
  };

  static const string NO_IP = string("N/A");
  static const string NO_MAC = string("N/A");
  static const string NET_PATH = "/sys/class/net/";
  static const string VIRTUAL_PATH = "/sys/devices/virtual/";

  static bool is_virtual(const std::string& ifname) {
    char* target = realpath((NET_PATH + ifname).c_str(), nullptr);

    if (!target) {
      throw system_error("realpath");
    }

    const std::string real_path{target};
    free(target);
    return real_path.rfind(VIRTUAL_PATH, 0) == 0;
  }

  NetType iface_type(const std::string& ifname) {
    if (file_util::exists(NET_PATH + ifname + "/wireless")) {
      return NetType::WIRELESS;
    }

    if (is_virtual(ifname)) {
      return NetType::OTHER;
    }

    return NetType::ETHERNET;
  }

  bool is_interface_valid(const string& ifname) {
    return if_nametoindex(ifname.c_str()) != 0;
  }

  /**
   * Returns the canonical name of the given interface and whether that differs
   * from the given name.
   *
   * The canonical name is the name that has an entry in `/sys/class/net`.
   *
   * This resolves any altnames that were defined (see man ip-link).
   */
  std::pair<string, bool> get_canonical_interface(const string& ifname) {
    int idx = if_nametoindex(ifname.c_str());

    if (idx == 0) {
      throw system_error("if_nameindex(" + ifname + ")");
    }

    char canonical[IF_NAMESIZE];
    if (!if_indextoname(idx, canonical)) {
      throw system_error("if_indextoname(" + to_string(idx) + ")");
    }

    string str{canonical};

    return {str, str != ifname};
  }

  /**
   * Test if interface with given name is a wireless device
   */
  bool is_wireless_interface(const string& ifname) {
    return iface_type(ifname) == NetType::WIRELESS;
  }

  std::string find_interface(NetType type) {
    struct ifaddrs* ifaddrs;
    getifaddrs(&ifaddrs);

    struct ifaddrs* candidate_if = nullptr;

    for (struct ifaddrs* i = ifaddrs; i != nullptr; i = i->ifa_next) {
      const std::string name{i->ifa_name};
      const NetType iftype = iface_type(name);
      if (iftype != type) {
        continue;
      }
      if (candidate_if == nullptr) {
        candidate_if = i;
      } else if (((candidate_if->ifa_flags & IFF_RUNNING) == 0) && ((i->ifa_flags & IFF_RUNNING) > 0)) {
        candidate_if = i;
      }
    }
    if (candidate_if) {
      const std::string name{candidate_if->ifa_name};
      freeifaddrs(ifaddrs);
      return name;
    }
    freeifaddrs(ifaddrs);
    return "";
  }

  std::string find_wireless_interface() {
    return find_interface(NetType::WIRELESS);
  }

  std::string find_wired_interface() {
    return find_interface(NetType::ETHERNET);
  }

  // class : network {{{

  /**
   * Construct network interface
   */
  network::network(string interface) : m_log(logger::make()), m_interface(move(interface)) {
    assert(is_interface_valid(m_interface));

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
    m_status.previous = m_status.current;
    m_status.current.transmitted = 0;
    m_status.current.received = 0;
    m_status.current.time = std::chrono::steady_clock::now();
    m_status.ip = NO_IP;
    m_status.ip6 = NO_IP;

    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1 || ifaddr == nullptr) {
      return false;
    }

    m_status.mac = string_util::trim(file_util::contents(NET_PATH + m_interface + "/address"), isspace);
    if (m_status.mac == "") {
      m_status.mac = NO_MAC;
    }

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
      command<output_policy::IGNORED> ping(m_log, exec);
      return ping.exec(true) == EXIT_SUCCESS;
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
   * Get interface mac address
   */
  string network::mac() const {
    return m_status.mac;
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
  string network::downspeed(int minwidth, const string& unit) const {
    float bytes_diff = m_status.current.received - m_status.previous.received;
    return format_speedrate(bytes_diff, minwidth, unit);
  }

  /**
   * Get upload speed rate
   */
  string network::upspeed(int minwidth, const string& unit) const {
    float bytes_diff = m_status.current.transmitted - m_status.previous.transmitted;
    return format_speedrate(bytes_diff, minwidth, unit);
  }

  /**
   * Get total net speed rate
   */
  string network::netspeed(int minwidth, const string& unit) const {
    float bytes_diff = m_status.current.received - m_status.previous.received + m_status.current.transmitted -
                       m_status.previous.transmitted;
    return format_speedrate(bytes_diff, minwidth, unit);
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
    auto operstate = file_util::contents(NET_PATH + m_interface + "/operstate");
    bool up = operstate.compare(0, 2, "up") == 0;
    return m_unknown_up ? (up || operstate.compare(0, 7, "unknown") == 0) : up;
  }

  /**
   * Format up- and download speed
   */
  string network::format_speedrate(float bytes_diff, int minwidth, const string& unit) const {
    // Get time difference in seconds as a float
    const std::chrono::duration<float> duration = m_status.current.time - m_status.previous.time;
    float time_diff = duration.count();
    float speedrate = bytes_diff / time_diff;

    vector<pair<string, int>> units{make_pair("G", 2), make_pair("M", 1)};
    string suffix{"K"};
    int precision = 0;

    while ((speedrate /= 1000) > 999) {
      suffix = units.back().first;
      precision = units.back().second;
      units.pop_back();
    }

    return sstream() << std::setw(minwidth) << std::setfill(' ') << std::setprecision(precision) << std::fixed
                     << speedrate << " " << suffix << unit;
  }

  // }}}
  // class : wired_network {{{

  /**
   * Query device driver for information
   */
  bool wired_network::query(bool accumulate) {
    if (!network::query(accumulate)) {
      return false;
    }

    if (m_tuntap) {
      return true;
    }

    if (m_bridge) {
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
      m_linkspeed = -1;
    } else {
      m_linkspeed = data.speed;
    }

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
    return m_linkspeed == -1 ? "N/A"
                             : (m_linkspeed < 1000 ? (to_string(m_linkspeed) + " Mbit/s")
                                                   : (to_string(m_linkspeed / 1000) + " Gbit/s"));
  }

  // }}}

} // namespace net

POLYBAR_NS_END
