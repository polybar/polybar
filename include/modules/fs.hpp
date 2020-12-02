#pragma once

#include "components/config.hpp"
#include "modules/meta/timer_module.hpp"
#include "settings.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Filesystem structure
   */
  struct fs_mount {
    string mountpoint;
    bool mounted = false;

    string type;
    string fsname;
    label_t label;

    uint64_t bytes_free{0ULL};
    uint64_t bytes_used{0ULL};
    uint64_t bytes_avail{0ULL};
    uint64_t bytes_total{0ULL};

    int percentage_free{0};
    int percentage_used{0};

    explicit fs_mount(string mountpoint, label_t label = nullptr, bool mounted = false)
        : mountpoint(move(mountpoint)), mounted(mounted), label(move(label)) {}
  };

  using fs_mount_t = unique_ptr<fs_mount>;

  /**
   * Module used to display filesystem stats.
   */
  class fs_module : public timer_module<fs_module> {
   public:
    explicit fs_module(const bar_settings&, string);

    bool update();
    string get_format() const;
    string get_output();
    bool build(builder* builder, const string& tag) const;

    static constexpr auto TYPE = "internal/fs";

   private:
    static constexpr auto FORMAT_MOUNTED = "format-mounted";
    static constexpr auto FORMAT_UNMOUNTED = "format-unmounted";
    static constexpr auto TAG_LABEL_MOUNTED = "<label-mounted>";
    static constexpr auto TAG_LABEL_UNMOUNTED = "<label-unmounted>";
    static constexpr auto TAG_BAR_USED = "<bar-used>";
    static constexpr auto TAG_BAR_FREE = "<bar-free>";
    static constexpr auto TAG_RAMP_CAPACITY = "<ramp-capacity>";

    label_t m_labelmounted;
    label_t m_labelunmounted;
    progressbar_t m_barused;
    progressbar_t m_barfree;
    ramp_t m_rampcapacity;

    label_t m_default_mounted_name;
    label_t m_default_unmounted_name;

    vector<string> m_mountpoints;
    vector<label_t> m_mountpoints_label_mounted{};
    vector<label_t> m_mountpoints_label_unmounted{};
    vector<fs_mount_t> m_mounts;
    bool m_fixed{false};
    bool m_remove_unmounted{false};
    int m_spacing{2};

    // used while formatting output
    size_t m_index{0_z};
  };
}  // namespace modules

POLYBAR_NS_END
