#pragma once

#include <cppunit/extensions/HelperMacros.h>

class unit_test : public CppUnit::TestFixture {
 public:
  virtual void prepare() {}
  virtual void finish() {}

  void set_up() {
    this->prepare();
  }

  void tear_down() {
    this->finish();
  }
};
