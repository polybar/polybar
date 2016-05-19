#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <signal.h>

#include "config.hpp"
#include "services/logger.hpp"
#include "interfaces/net.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

using namespace net;

bool net::is_wireless_interface(const std::string& ifname) {
  return io::file::exists("/sys/class/net/"+ ifname +"/wireless");
}


// Network

Network::Network(const std::string& interface) throw(NetworkException)
{
  this->interface = interface;

  if (if_nametoindex(this->interface.c_str()) == 0)
    throw NetworkException("Invalid network interface \""+ this->interface +"\"");

  if ((this->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    throw NetworkException("Failed to open socket: "+ STRERRNO);

  std::memset(&this->data, 0, sizeof(this->data));
  std::strncpy(this->data.ifr_name, this->interface.data(), IFNAMSIZ-1);
}

Network::~Network()
{
  if (close(this->fd) == -1)
    log_error("Failed to close Network socket FD: "+ STRERRNO);
}

bool Network::test_interface() throw(NetworkException)
{
  if ((ioctl(this->fd, SIOCGIFFLAGS, &this->data)) == -1)
    throw NetworkException(STRERRNO);
  return this->data.ifr_flags & IFF_UP;
}

bool Network::test_connection() throw(NetworkException) {
  int status = EXIT_FAILURE;

  try {
    this->ping = std::make_unique<Command>(
        "ping -c 2 -W 2 -I "+ this->interface +" " + std::string(CONNECTION_TEST_IP));

    status = this->ping->exec(true);

    this->ping.reset();
  } catch (CommandException &e) {
    log_error(e.what());
  } catch (proc::ExecFailure &e) {
    log_error(e.what());
  }

  return (status == EXIT_SUCCESS);
}

bool Network::test()
{
  try {
    return this->test_interface() && this->test_connection();
  } catch (NetworkException &e) {
    return false;
  }
}

bool Network::connected()
{
  try {
    return this->test_interface() &&
      io::file::get_contents("/sys/class/net/"+ this->interface +"/carrier")[0] == '1';
  } catch (NetworkException &e) {
    return false;
  }
}

std::string Network::get_ip() throw(NetworkException)
{
  if (!this->test_interface())
    throw NetworkException("Interface is not up");

  this->data.ifr_addr.sa_family = AF_INET;

  if (ioctl(this->fd, SIOCGIFADDR, &this->data) == -1)
    throw NetworkException(STRERRNO);

  return inet_ntoa(((struct sockaddr_in *) &this->data.ifr_addr)->sin_addr);
}


// WiredNetwork

WiredNetwork::WiredNetwork(const std::string& interface) : Network(interface)
{
  struct ethtool_cmd e;

  e.cmd = ETHTOOL_GSET;

  this->data.ifr_data = (caddr_t) &e;

  if (ioctl(this->fd, SIOCETHTOOL, &this->data) == 0)
    this->linkspeed = (e.speed == USHRT_MAX ? 0 : e.speed);
}

std::string WiredNetwork::get_link_speed() {
  return std::string((this->linkspeed == 0 ? "???" : std::to_string(this->linkspeed)) +" Mbit/s");
}


// WirelessNetwork

WirelessNetwork::WirelessNetwork(const std::string& interface) : Network(interface) {
  std::strcpy((char *) &this->iw.ifr_ifrn.ifrn_name, this->interface.c_str());
}

std::string WirelessNetwork::get_essid() throw(WirelessNetworkException)
{
  char essid[IW_ESSID_MAX_SIZE+1];

  std::memset(&essid, 0, IW_ESSID_MAX_SIZE + 1);

  this->iw.u.essid.pointer = &essid;
  this->iw.u.essid.length = sizeof(essid);

  if (ioctl(this->fd, SIOCGIWESSID, &this->iw) == -1)
    throw WirelessNetworkException(STRERRNO);

  return string::trim(essid, ' ');
}

float WirelessNetwork::get_signal_quality()
{
  auto dbm = this->get_signal_dbm();
  return 2 * (dbm + 100);
}

float WirelessNetwork::get_signal_dbm() throw(WirelessNetworkException)
{
  this->iw.u.data.pointer = (iw_statistics *) std::malloc(sizeof(iw_statistics));
  this->iw.u.data.length = sizeof(iw_statistics);

  if (ioctl(this->fd, SIOCGIWSTATS, &this->iw) == -1)
    throw WirelessNetworkException(STRERRNO);

  auto signal = ((iw_statistics *) this->iw.u.data.pointer)->qual.level - 256;

  std::free(this->iw.u.data.pointer);

  return signal;
}
