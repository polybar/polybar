//
// Copyright (c) 2016 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstdio>
#include <cstdlib>
#include <gtest/gtest.h>

#define expect(...) \
  (void)((__VA_ARGS__) || (expect_fail__(#__VA_ARGS__, __FILE__, __LINE__), 0))
#define static_expect(...) static_assert((__VA_ARGS__), "fail")

void expect_fail__(const char* msg, const char* file, int line) {
  std::printf("%s:%d:%s\n", file, line, msg);
  std::exit(-1);
}

template <char...>
struct test {
  template <class Test>
  bool operator=(const Test& test) {
    test();
    return true;
  }
};

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

template <class T, T... Chars>
constexpr auto operator""_test() {
  return test<Chars...>{};
}
