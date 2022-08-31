#pragma once

#include <tuple>
#include <vector>

#include "common.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

using std::tuple;

POLYBAR_NS

namespace drawtypes {
  class layouticonset : public non_copyable_mixin {
   public:
    explicit layouticonset(label_t&& default_icon);

    bool add(const string& layout, const string& variant, label_t&& icon);
    label_t get(const string& layout, const string& variant) const;
    bool contains(const string& layout, const string& variant) const;

    static constexpr const char* VARIANT_ANY = "_";

   protected:
    label_t m_default_icon;
    vector<tuple<string, string, label_t>> m_layout_icons;
  };

  using layouticonset_t = shared_ptr<layouticonset>;
}  // namespace drawtypes

POLYBAR_NS_END
