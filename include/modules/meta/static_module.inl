POLYBAR_NS

namespace modules {
  template <typename Impl>
  void static_module<Impl>::start() {
    CAST_MOD(Impl)->broadcast();
  }

  template <typename Impl>
  bool static_module<Impl>::build(builder*, string) const {
    return true;
  }
}

POLYBAR_NS_END
