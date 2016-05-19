#include "drawtypes/ramp.hpp"
#include "utils/config.hpp"
#include "utils/memory.hpp"

namespace drawtypes
{
  std::unique_ptr<Ramp> get_config_ramp(const std::string& config_path, const std::string& ramp_name, bool required)
  {
    std::vector<std::unique_ptr<Icon>> vec;
    std::vector<std::string> icons;

    if (required)
      icons = config::get_list<std::string>(config_path, ramp_name);
    else
      icons = config::get_list<std::string>(config_path, ramp_name, {});

    auto n_icons = icons.size();
    repeat(n_icons)
    {
      auto ramp = ramp_name +":"+ std::to_string(repeat_i_rev(n_icons));
      vec.emplace_back(std::unique_ptr<Icon> { get_optional_config_icon(config_path, ramp, icons[repeat_i_rev(n_icons)]) });
    }

    return std::unique_ptr<Ramp> { new Ramp(std::move(vec)) };
  }

  Ramp::Ramp(std::vector<std::unique_ptr<Icon>> icons) {
    this->icons.swap(icons);
  }

  void Ramp::add(std::unique_ptr<Icon> &&icon) {
    this->icons.emplace_back(std::move(icon));
  }

  std::unique_ptr<Icon> &Ramp::get(int idx) {
    return this->icons[idx];
  }

  std::unique_ptr<Icon> &Ramp::get_by_percentage(float percentage) {
    return this->icons[(int)(percentage * (this->icons.size() - 1) / 100.0 + 0.5)];
  }
}
