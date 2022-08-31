#include <linux/nl80211.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

#include <algorithm>

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

    struct nl_sock* sk = nl_socket_alloc();
    if (sk == nullptr) {
      return false;
    }

    if (genl_connect(sk) < 0) {
      return false;
    }

    int driver_id = genl_ctrl_resolve(sk, "nl80211");
    if (driver_id < 0) {
      nl_socket_free(sk);
      return false;
    }

    if (nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM, scan_cb, this) != 0) {
      nl_socket_free(sk);
      return false;
    }

    struct nl_msg* msg = nlmsg_alloc();
    if (msg == nullptr) {
      nl_socket_free(sk);
      return false;
    }

    if ((genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0) == nullptr) ||
        nla_put_u32(msg, NL80211_ATTR_IFINDEX, m_ifid) < 0) {
      nlmsg_free(msg);
      nl_socket_free(sk);
      return false;
    }

    // nl_send_sync always frees msg
    if (nl_send_sync(sk, msg) < 0) {
      nl_socket_free(sk);
      return false;
    }

    nl_socket_free(sk);

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
   * Callback to parse scan results
   */
  int wireless_network::scan_cb(struct nl_msg* msg, void* instance) {
    auto wn = static_cast<wireless_network*>(instance);
    auto gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));
    struct nlattr* tb[NL80211_ATTR_MAX + 1];
    struct nlattr* bss[NL80211_BSS_MAX + 1];

    struct nla_policy bss_policy[NL80211_BSS_MAX + 1]{};
    bss_policy[NL80211_BSS_TSF].type = NLA_U64;
    bss_policy[NL80211_BSS_FREQUENCY].type = NLA_U32;
    bss_policy[NL80211_BSS_BSSID].type = NLA_UNSPEC;
    bss_policy[NL80211_BSS_BEACON_INTERVAL].type = NLA_U16;
    bss_policy[NL80211_BSS_CAPABILITY].type = NLA_U16;
    bss_policy[NL80211_BSS_INFORMATION_ELEMENTS].type = NLA_UNSPEC;
    bss_policy[NL80211_BSS_SIGNAL_MBM].type = NLA_U32;
    bss_policy[NL80211_BSS_SIGNAL_UNSPEC].type = NLA_U8;
    bss_policy[NL80211_BSS_STATUS].type = NLA_U32;

    if (nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), nullptr) < 0) {
      return NL_SKIP;
    }

    if (tb[NL80211_ATTR_BSS] == nullptr) {
      return NL_SKIP;
    }

    if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy) != 0) {
      return NL_SKIP;
    }

    if (!wn->associated_or_joined(bss)) {
      return NL_SKIP;
    }

    wn->parse_essid(bss);
    wn->parse_frequency(bss);
    wn->parse_signal(bss);
    wn->parse_quality(bss);

    return NL_SKIP;
  }

  /**
   * Check for a connection to a AP
   */
  bool wireless_network::associated_or_joined(struct nlattr** bss) {
    if (bss[NL80211_BSS_STATUS] == nullptr) {
      return false;
    }

    auto status = nla_get_u32(bss[NL80211_BSS_STATUS]);

    switch (status) {
      case NL80211_BSS_STATUS_ASSOCIATED:
      case NL80211_BSS_STATUS_IBSS_JOINED:
      case NL80211_BSS_STATUS_AUTHENTICATED:
        return true;
      default:
        return false;
    }
  }

  /**
   * Set the ESSID
   */
  void wireless_network::parse_essid(struct nlattr** bss) {
    m_essid.clear();

    if (bss[NL80211_BSS_INFORMATION_ELEMENTS] != nullptr) {
// Information Element ID from ieee80211.h
#define WLAN_EID_SSID 0

      auto ies = static_cast<char*>(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
      auto ies_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
      const auto hdr_len = 2;

      while (ies_len > hdr_len && ies[0] != WLAN_EID_SSID) {
        ies_len -= ies[1] + hdr_len;
        ies += ies[1] + hdr_len;
      }

      if (ies_len > hdr_len && ies_len > ies[1] + hdr_len) {
        auto essid_begin = ies + hdr_len;
        auto essid_end = essid_begin + ies[1];

        std::copy(essid_begin, essid_end, std::back_inserter(m_essid));
      }
    }
  }

  /**
   * Set frequency
   */
  void wireless_network::parse_frequency(struct nlattr** bss) {
    if (bss[NL80211_BSS_FREQUENCY] != nullptr) {
      // in MHz
      m_frequency = static_cast<int>(nla_get_u32(bss[NL80211_BSS_FREQUENCY]));
    }
  }

  /**
   * Set device driver quality values
   */
  void wireless_network::parse_quality(struct nlattr** bss) {
    if (bss[NL80211_BSS_SIGNAL_UNSPEC] != nullptr) {
      // Signal strength in unspecified units, scaled to 0..100 (u8)
      m_linkquality.val = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
      m_linkquality.max = 100;
    }
  }

  /**
   * Set the signalstrength
   */
  void wireless_network::parse_signal(struct nlattr** bss) {
    if (bss[NL80211_BSS_SIGNAL_MBM] != nullptr) {
      // signalstrength in dBm
      int signalstrength = static_cast<int>(nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM])) / 100;

      // WiFi-hardware usually operates in the range -90 to -20dBm.
      const int hardware_max = -20;
      const int hardware_min = -90;
      signalstrength = std::max(hardware_min, std::min(signalstrength, hardware_max));

      // Shift for positive values
      m_signalstrength.val = signalstrength - hardware_min;
      m_signalstrength.max = hardware_max - hardware_min;
    }
  }
} // namespace net

POLYBAR_NS_END
