#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  class labellist : public non_copyable_mixin<labellist> {
   public:
    labellist() {}
    explicit labellist(vector<label_t>&& labels, label_t&& tmplate)
      : m_labels(move(labels))
      , m_template(move(tmplate)) {}
    label_t& get_template();
    void apply_template();

   protected:
    vector<label_t> m_labels;
    label_t m_template;

  };

  using labellist_t = shared_ptr<labellist>;
  void load_labellist(vector<label_t>& labels, label_t& tmplate, const config& conf, const string& section, string name, bool required = true);
}

POLYBAR_NS_END
