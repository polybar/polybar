#pragma once

#include <unordered_map>

#include "common.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

template <typename ValueType, typename KeyType>
class cache {
 public:
  using map_type = std::unordered_map<KeyType, std::weak_ptr<ValueType>>;
  using safe_map_type = mutex_wrapper<map_type>;

  bool check(const KeyType& key) {
    std::lock_guard<safe_map_type> guard(m_cache);
    return m_cache.find(key) == m_cache.end();
  }

  template <typename... MakeArgs>
  shared_ptr<ValueType> object(const KeyType& key, MakeArgs&&... make_args) {
    std::lock_guard<safe_map_type> guard(m_cache);
    auto ptr = m_cache[key].lock();
    if (!ptr) {
      m_cache[key] = ptr = make_shared<ValueType>(forward<MakeArgs>(make_args)...);
    }
    return ptr;
  }

 private:
  safe_map_type m_cache;
};

POLYBAR_NS_END
