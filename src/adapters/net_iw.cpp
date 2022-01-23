#include "adapters/net.hpp"

#include "utils/file.hpp"

POLYBAR_NS

namespace net {
  // class : wireless_network {{{

  /**
   * Query the wireless device for information
   * about the current connection
   */
  bool wireless_network::query(bool accumulate) {
    if (!network::query(accumulate)) {
      return false;
    }

    auto socket_fd = file_util::make_file_descriptor(iw_sockets_open());
    if (!*socket_fd) {
      return false;
    }

    struct iwreq req {};

    if (iw_get_ext(*socket_fd, m_interface.c_str(), SIOCGIWMODE, &req) == -1) {
      return false;
    }

    // Ignore interfaces in ad-hoc mode
    if (req.u.mode == IW_MODE_ADHOC) {
      return false;
    }

    query_essid(*socket_fd);
    query_quality(*socket_fd);

    return true;
  }

  /**
   * Check current connection state
   */
  bool wireless_network::connected() const {
    if (!network::test_interface()) {
      return false;
    }
    return !m_essid.empty();
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

    struct iwreq req {};
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
    iwrange range{};
    iwstats stats{};

    // Fill range
    if (iw_get_range_info(socket_fd, m_interface.c_str(), &range) == -1) {
      return;
    }
    // Fill stats
    if (iw_get_stats(socket_fd, m_interface.c_str(), &stats, &range, 1) == -1) {
      return;
    }

    // Check if the driver supplies the quality value
    if (stats.qual.updated & IW_QUAL_QUAL_INVALID) {
      return;
    }
    // Check if the driver supplies the quality level value
    if (stats.qual.updated & IW_QUAL_LEVEL_INVALID) {
      return;
    }

    // Check if the link quality has been updated
    if (stats.qual.updated & IW_QUAL_QUAL_UPDATED) {
      m_linkquality.val = stats.qual.qual;
      m_linkquality.max = range.max_qual.qual;
    }

    // Check if the signal strength has been updated
    if (stats.qual.updated & IW_QUAL_LEVEL_UPDATED) {
      m_signalstrength.val = stats.qual.level;
      m_signalstrength.max = range.max_qual.level;

      // Check if the values are defined in dBm
      if (stats.qual.level > range.max_qual.level) {
        m_signalstrength.val -= 0x100;
        m_signalstrength.max = (stats.qual.level - range.max_qual.level) - 0x100;
      }
    }
  }

  // }}}
}  // namespace net

POLYBAR_NS_END
