#pragma once

#include "common.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

class http_downloader : public non_copyable_mixin, public non_movable_mixin {
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

POLYBAR_NS_END
