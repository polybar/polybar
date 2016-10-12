#pragma once

#include <bitset>
#include <iomanip>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iwlib.h>
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
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <memory>
#include <sstream>
#include <string>
#include <string>

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
  DEFINE_ERROR(network_error);
  DEFINE_ERROR(wired_network_error);
  DEFINE_ERROR(wireless_network_error);

  // types {{{

  struct bytes_t {
    uint32_t transmitted = 0;
    uint32_t received = 0;
    std::chrono::system_clock::time_point time;
  };

  struct linkdata_t {
    string ip_address;
    bytes_t previous;
    bytes_t current;
  };

  // }}}
  // class: network {{{

  class network {
   public:
    explicit network(string interface) : m_interface(interface) {
      if (if_nametoindex(m_interface.c_str()) == 0)
        throw network_error("Invalid network interface \"" + m_interface + "\"");
      if ((m_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw network_error("Failed to open socket");
      std::memset(&m_data, 0, sizeof(m_data));
      std::strncpy(m_data.ifr_name, m_interface.data(), IFNAMSIZ - 1);
    }

    ~network() {
      if (m_fd != -1)
        close(m_fd);
    }

    bool test_interface() {
      if ((ioctl(m_fd, SIOCGIFFLAGS, &m_data)) == -1)
        throw network_error("Failed to get flags");
      if ((m_data.ifr_flags & IFF_UP) == 0)
        return false;
      if ((m_data.ifr_flags & IFF_RUNNING) == 0)
        return false;
      return true;
    }

    bool test_connection() {
      int status = EXIT_FAILURE;

      try {
        m_ping = command_util::make_command(
            "ping -c 2 -W 2 -I " + m_interface + " " + string(CONNECTION_TEST_IP));
        status = m_ping->exec(true);
        m_ping.reset();
      } catch (std::exception& e) {
      }

      return (status == EXIT_SUCCESS);
    }

    bool test() {
      try {
        return test_interface() && test_connection();
      } catch (network_error& e) {
        return false;
      }
    }

    bool connected() {
      try {
        if (!test_interface())
          return false;
        return file_util::get_contents("/sys/class/net/" + m_interface + "/carrier")[0] == '1';
      } catch (network_error& e) {
        return false;
      }
    }

    bool query_interface() {
      auto now = chrono::system_clock::now();
      if ((now - m_last_query) < chrono::seconds(1))
        return true;
      m_last_query = now;

      struct ifaddrs* ifaddr;
      getifaddrs(&ifaddr);
      bool match = false;

      for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (m_interface.compare(0, m_interface.length(), ifa->ifa_name) != 0)
          continue;
        match = true;

        switch (ifa->ifa_addr->sa_family) {
          case AF_INET:
            char ip_buffer[NI_MAXHOST];
            getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), ip_buffer, NI_MAXHOST, nullptr, 0,
                NI_NUMERICHOST);
            m_linkdata.ip_address = string(ip_buffer);
            break;

          case AF_PACKET:
            if (ifa->ifa_data == nullptr)
              continue;
            struct rtnl_link_stats* link_state =
                reinterpret_cast<struct rtnl_link_stats*>(ifa->ifa_data);
            m_linkdata.previous = m_linkdata.current;
            m_linkdata.current.transmitted = link_state->tx_bytes;
            m_linkdata.current.received = link_state->rx_bytes;
            m_linkdata.current.time = chrono::system_clock::now();
            break;
        }
      }

      freeifaddrs(ifaddr);

      return match;
    }

    string ip() {
      if (!test_interface())
        throw network_error("Interface is not up");
      if (!query_interface())
        throw network_error("Failed to query interface");
      return m_linkdata.ip_address;
    }

    string downspeed(int minwidth = 3) {
      if (!query_interface())
        throw network_error("Failed to query interface");

      float bytes_diff = m_linkdata.current.received - m_linkdata.previous.received;
      float time_diff =
          chrono::duration_cast<chrono::seconds>(m_linkdata.current.time - m_linkdata.previous.time)
              .count();
      float speed = bytes_diff / time_diff;

      speed /= 1000;  // convert to KB
      int suffix_n = 0;
      vector<string> suffixes{"KB", "MB", "GB"};

      while (speed >= 1000 && suffix_n < (int)suffixes.size() - 1) {
        suffix_n++;
        speed /= 1000;
      }

      return string_util::from_stream(stringstream() << std::setw(minwidth) << std::setfill(' ')
                                                     << std::setprecision(0) << std::fixed << speed
                                                     << " " << suffixes[suffix_n] << "/s");
    }

    string upspeed(int minwidth = 3) {
      if (!query_interface())
        throw network_error("Failed to query interface");

      float bytes_diff = m_linkdata.current.transmitted - m_linkdata.previous.transmitted;
      float time_diff =
          chrono::duration_cast<chrono::seconds>(m_linkdata.current.time - m_linkdata.previous.time)
              .count();
      float speed = bytes_diff / time_diff;

      speed /= 1000;  // convert to KB
      int suffix_n = 0;
      vector<string> suffixes{"KB", "MB", "GB"};

      while (speed >= 1000 && suffix_n < (int)suffixes.size() - 1) {
        suffix_n++;
        speed /= 1000;
      }

      return string_util::from_stream(stringstream() << std::setw(minwidth) << std::setfill(' ')
                                                     << std::setprecision(0) << std::fixed << speed
                                                     << " " << suffixes[suffix_n] << "/s");
    }

   protected:
    unique_ptr<command_util::command> m_ping;
    string m_interface;
    string m_ip;
    struct ifreq m_data;
    int m_fd = 0;

    linkdata_t m_linkdata;

    chrono::system_clock::time_point m_last_query;
  };

  // }}}
  // class: wired_network {{{

  class wired_network : public network {
   public:
    explicit wired_network(string interface) : network(interface) {
      struct ethtool_cmd e;
      e.cmd = ETHTOOL_GSET;

      m_data.ifr_data = (caddr_t)&e;

      if (ioctl(m_fd, SIOCETHTOOL, &m_data) == 0)
        m_linkspeed = (e.speed == USHRT_MAX ? 0 : e.speed);
    }

    string link_speed() {
      return string((m_linkspeed == 0 ? "???" : to_string(m_linkspeed)) + " Mbit/s");
    }

   private:
    int m_linkspeed = 0;
  };

  // }}}
  // class: wireless_network {{{

  struct wireless_info {
    std::bitset<5> flags;
    string essid{IW_ESSID_MAX_SIZE + 1};
    int quality = 0;
    int quality_max = 0;
    int quality_avg = 0;
    int signal = 0;
    int signal_max = 0;
    int noise = 0;
    int noise_max = 0;
    int bitrate = 0;
    double frequency = 0;
  };

  enum wireless_flags {
    ESSID = 0,
    QUALITY = 1,
    SIGNAL = 2,
    NOISE = 3,
    FREQUENCY = 4,
  };

  class wireless_network : public network {
   public:
    wireless_network(string interface) : network(interface) {
      std::strcpy((char*)&m_iw.ifr_ifrn.ifrn_name, m_interface.c_str());

      if (!m_info)
        m_info.reset(new wireless_info());
    }

    string essid() {
      if (!query_interface())
        return "";
      if (!m_info->flags.test(wireless_flags::ESSID))
        return "";
      return m_info->essid;
    }

    float signal_quality() {
      if (!query_interface())
        return 0;
      if (m_info->flags.test(wireless_flags::QUALITY))
        return 2 * (signal_dbm() + 100);
      return 0;
    }

    float signal_dbm() {
      if (!query_interface())
        return 0;
      if (m_info->flags.test(wireless_flags::QUALITY))
        return m_info->quality + m_info->noise - 256;
      return 0;
    }

   protected:
    bool query_interface() {
      if ((chrono::system_clock::now() - m_last_query) < chrono::seconds(1))
        return true;

      network::query_interface();

      auto ifname = m_interface.c_str();
      auto socket_fd = iw_sockets_open();

      if (socket_fd == -1)
        return false;

      auto on_exit = scope_util::make_exit_handler<>([&]() { iw_sockets_close(socket_fd); });
      {
        wireless_config wcfg;

        if (iw_get_basic_config(socket_fd, ifname, &wcfg) == -1)
          return false;

        // reset flags
        m_info->flags.none();

        if (wcfg.has_essid && wcfg.essid_on) {
          m_info->essid = {wcfg.essid, 0, IW_ESSID_MAX_SIZE};
          m_info->flags |= wireless_flags::ESSID;
        }

        if (wcfg.has_freq) {
          m_info->frequency = wcfg.freq;
          m_info->flags |= wireless_flags::FREQUENCY;
        }

        if (wcfg.mode == IW_MODE_ADHOC)
          return true;

        iwrange range;
        if (iw_get_range_info(socket_fd, ifname, &range) == -1)
          return false;

        iwstats stats;
        if (iw_get_stats(socket_fd, ifname, &stats, &range, 1) == -1)
          return false;

        if (stats.qual.updated & IW_QUAL_RCPI) {
          if (!(stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
            m_info->quality = stats.qual.qual;
            m_info->quality_max = range.max_qual.qual;
            m_info->quality_avg = range.avg_qual.qual;
            m_info->flags |= wireless_flags::QUALITY;
          }

          if (stats.qual.updated & IW_QUAL_RCPI) {
            if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
              m_info->signal = stats.qual.level / 2.0 - 110 + 0.5;
              m_info->flags |= wireless_flags::SIGNAL;
            }
            if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
              m_info->noise = stats.qual.noise / 2.0 - 110 + 0.5;
              m_info->flags |= wireless_flags::NOISE;
            }
          } else {
            if ((stats.qual.updated & IW_QUAL_DBM) || stats.qual.level > range.max_qual.level) {
              if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
                m_info->signal = stats.qual.level;
                if (m_info->signal > 63)
                  m_info->signal -= 256;
                m_info->flags |= wireless_flags::SIGNAL;
              }
              if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
                m_info->noise = stats.qual.noise;
                if (m_info->noise > 63)
                  m_info->noise -= 256;
                m_info->flags |= wireless_flags::NOISE;
              }
            } else {
              if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
                m_info->signal = stats.qual.level;
                m_info->signal_max = range.max_qual.level;
                m_info->flags |= wireless_flags::SIGNAL;
              }
              if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
                m_info->noise = stats.qual.noise;
                m_info->noise_max = range.max_qual.noise;
                m_info->flags |= wireless_flags::NOISE;
              }
            }
          }
        } else {
          if (!(stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
            m_info->quality = stats.qual.qual;
            m_info->flags |= wireless_flags::QUALITY;
          }
          if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
            m_info->quality = stats.qual.level;
            m_info->flags |= wireless_flags::SIGNAL;
          }
          if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
            m_info->quality = stats.qual.noise;
            m_info->flags |= wireless_flags::NOISE;
          }
        }

        // struct iwreq wrq;
        // if (iw_get_ext(socket_fd, ifname, SIOCGIWRATE, &wrq) != -1)
        //   m_info->bitrate = wrq.u.bitrate.value;

        return true;
      }
    }

   private:
    struct iwreq m_iw;
    shared_ptr<wireless_info> m_info;
  };

  // }}}

  inline bool is_wireless_interface(string ifname) {
    return file_util::exists("/sys/class/net/" + ifname + "/wireless");
  }
}

LEMONBUDDY_NS_END
