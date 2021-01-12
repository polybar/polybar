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
    explicit labellist(vector<label_t>&& labels)
      : m_labels(move(labels)) {}
    void reset_tokens();
    void replace_token(const string& token, const string& replacement);
    label_t get_template();
    void apply_template();

   protected:
    vector<label_t> m_labels;

  };

  using labellist_t = shared_ptr<labellist>;
  void load_labellist(vector<label_t>& labels, const config& conf, const string& section, string name, bool required = true);
}

POLYBAR_NS_END
