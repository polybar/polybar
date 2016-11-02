#pragma once

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iwlib.h>

#ifdef inline
#undef inline
#endif

#include "common.hpp"
#include "config.hpp"

LEMONBUDDY_NS

namespace net {
  DEFINE_ERROR(network_error);

  bool is_wireless_interface(string ifname);

  // types {{{

  struct quality_range {
    int val = 0;
    int max = 0;

    int percentage() const {
      if (max < 0)
        return 2 * (val + 100);
      return static_cast<float>(val) / max * 100.0f + 0.5f;
    }
  };

  using bytes_t = unsigned int;

  struct link_activity {
    bytes_t transmitted = 0;
    bytes_t received = 0;
    chrono::system_clock::time_point time;
  };

  struct link_status {
    string ip;
    link_activity previous;
    link_activity current;
  };

  // }}}
  // class : network {{{

  class network {
   public:
    explicit network(string interface);
    virtual ~network();

    virtual bool query();
    virtual bool connected() const = 0;
    virtual bool ping() const;

    string ip() const;
    string downspeed(int minwidth = 3) const;
    string upspeed(int minwidth = 3) const;

   protected:
    bool test_interface() const;
    string format_speedrate(float bytes_diff, int minwidth) const;

    int m_socketfd = 0;
    link_status m_status;
    string m_interface;
  };

  // }}}
  // class : wired_network {{{

  class wired_network : public network {
   public:
    explicit wired_network(string interface) : network(interface) {}

    bool query() override;
    bool connected() const override;
    string linkspeed() const;

   private:
    int m_linkspeed = 0;
  };

  // }}}
  // class : wireless_network {{{

  class wireless_network : public network {
   public:
    wireless_network(string interface) : network(interface) {}

    bool query() override;
    bool connected() const override;

    string essid() const;
    int signal() const;
    int quality() const;

   protected:
    void query_essid(const int& socket_fd);
    void query_quality(const int& socket_fd);

   private:
    shared_ptr<wireless_info> m_info;
    string m_essid;
    quality_range m_signalstrength;
    quality_range m_linkquality;
  };

  // }}}

  using wireless_t = unique_ptr<wireless_network>;
  using wired_t = unique_ptr<wired_network>;
}

LEMONBUDDY_NS_END
