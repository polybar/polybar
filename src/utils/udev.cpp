#include "utils/udev.hpp"

#include <libudev.h>
#include <utils/udev.hpp>

POLYBAR_NS

udev_device udev_util::get_device(const std::string& sysname) {
  udev_device device;

  udev* udev = udev_connection::make().conn;
  udev_list_entry* dev_list_entry;
  udev_enumerate* enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_subsystem(enumerate, "backlight");
  udev_enumerate_scan_devices(enumerate);

  udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(dev_list_entry, devices) {
    const char* path = udev_list_entry_get_name(dev_list_entry);
    ::udev_device* dev = udev_device_new_from_syspath(udev, path);

    if (udev_device_get_sysname(dev) == sysname) {
      device = udev_device{dev};
      break;
    }

    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);

  return device;
}

udev_device::~udev_device() {
  udev_device_unref(m_dev);
}

udev_device::udev_device(udev_device&& rhs) noexcept : m_dev(std::exchange(rhs.m_dev, nullptr)) {}

udev_device& udev_device::operator=(udev_device&& rhs) noexcept {
  m_dev = std::exchange(rhs.m_dev, nullptr);
  return *this;
}

const char* udev_device::get_sysattr(const char* attr) const {
  return udev_device_get_sysattr_value(m_dev, attr);
}

const char* udev_device::get_sysattr(const std::string& attr) const {
  return get_sysattr(attr.c_str());
}

const char* udev_device::get_sysname() const {
  return udev_device_get_sysname(m_dev);
}

const char* udev_device::get_syspath() const {
  return udev_device_get_syspath(m_dev);
}

const char* udev_device::get_devtype() const {
  return udev_device_get_devtype(m_dev);
}

const char* udev_device::get_action() const {
  return udev_device_get_action(m_dev);
}

udev_event::udev_event(udev_device&& dev) : dev(move(dev)) {}

void udev_watch::attach() {
  if (udev_monitor_filter_add_match_subsystem_devtype(m_monitor, m_subsystem.c_str(), nullptr) < 0) {
    throw udev_error("Failed to add filter to udev monitor");
  }
  if (udev_monitor_enable_receiving(m_monitor) < 0) {
    throw udev_error("Failed to enabling udev monitor");
  }
  m_fd = udev_monitor_get_fd(m_monitor);
}

bool udev_watch::poll(int wait_ms) const {
  fd_set fds;
  timeval tv{0, wait_ms};
  int ret;

  FD_ZERO(&fds);
  FD_SET(m_fd, &fds);

  ret = ::select(m_fd + 1, &fds, nullptr, nullptr, &tv);
  return ret > 0 && FD_ISSET(m_fd, &fds);
}

polybar::udev_event udev_watch::get_event() const {
  return udev_event{udev_device{udev_monitor_receive_device(m_monitor)}};
}

udev_watch::~udev_watch() {
  udev_monitor_unref(m_monitor);
}

udev_watch::udev_watch(polybar::udev_connection& connection, std::string subsystem)
    : m_monitor(udev_monitor_new_from_netlink(connection.conn, "udev")), m_subsystem(move(subsystem)) {
  if (!m_monitor) {
    throw udev_error("Failde to create udev monitor");
  }
}

polybar::udev_connection& udev_connection::make() {
  return *factory_util::singleton<std::remove_reference_t<make_type>>();
}

udev_connection::udev_connection() : conn(udev_new()) {
  if (!conn) {
    throw udev_error("Failde to create udev connection");
  }
}

POLYBAR_NS_END
