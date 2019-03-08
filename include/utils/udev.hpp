#pragma once

#include <memory>

#include "common.hpp"
#include "errors.hpp"
#include "utils/factory.hpp"

struct udev;
struct udev_device;
struct udev_monitor;

POLYBAR_NS

DEFINE_ERROR(udev_error);

class udev_connection {
 public:
  using make_type = udev_connection&;

  static make_type make();

  udev_connection();

  udev* conn;
};

class udev_device {
 public:
  udev_device() = default;
  explicit udev_device(::udev_device* dev) noexcept : m_dev{dev} {}
  udev_device(const udev_device&) = delete;
  udev_device(udev_device&& rhs) noexcept;

  ~udev_device();

  udev_device& operator=(const udev_device&) noexcept = delete;
  udev_device& operator=(udev_device&& rhs) noexcept;

  /**
   * Returns true if the device is valid
   */
  explicit operator bool() const {
    return m_dev != nullptr;
  }

  /**
   * Returns the value of the given attribute.
   *
   * For example if the device if brightness card (like intel_backlight),
   * an attribute can be "actual_brightness" or "max_brightness"
   */
  const char* get_sysattr(const char* attr) const;

  /**
   * Overload for strings
   */
  const char* get_sysattr(const string& attr) const;

  /**
   * Returns the name of the device
   */
  const char* get_sysname() const;

  /**
   * Returns the path of the device
   */
  const char* get_syspath() const;

  /**
   * Returns the type of the device
   */
  const char* get_devtype() const;

 private:
  ::udev_device* m_dev = nullptr;
};
class udev_event {
 public:
  explicit udev_event(udev_device&& dev);

  udev_device dev;
};

class udev_watch {
 public:
  explicit udev_watch(udev_connection& connection, string subsystem);

  ~udev_watch();

  /**
   * Starts monitoring
   */
  void attach();

  bool poll(int wait_ms = 1000) const;

  udev_event get_event() const;

  const string& subsystem() const {
    return m_subsystem;
  }

 protected:
  udev_monitor* m_monitor;
  string m_subsystem;
  int m_fd = -1;
};

namespace udev_util {
  template <typename... Args>
  decltype(auto) make_watch(Args&&... args) {
    return factory_util::unique<udev_watch>(udev_connection::make(), forward<Args>(args)...);
  }

  /**
   * Returns the device with the given sysname or an empty device if it doesn't exist.
   */
  udev_device get_device(const string& sysname);
}  // namespace udev_util

POLYBAR_NS_END
