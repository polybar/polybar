#pragma once

#include "common.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

class http_downloader {
 public:
  http_downloader(int connection_timeout = 5);
  ~http_downloader();

  string get(const string& url, const string& user = "", const string& password = "");
  long response_code();

 protected:
  static size_t write(void* p, size_t size, size_t bytes, void* stream);

 private:
  void* m_curl;
};

namespace http_util {
  template <typename... Args>
  decltype(auto) make_downloader(Args&&... args) {
    return factory_util::unique<http_downloader>(forward<Args>(args)...);
  }
}  // namespace http_util

POLYBAR_NS_END
