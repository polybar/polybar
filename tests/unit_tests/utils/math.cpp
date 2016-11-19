#include "utils/math.hpp"

int main() {
  using namespace polybar;

  "cap"_test = [] {
    expect(math_util::cap<int>(8, 0, 10) == 8);
    expect(math_util::cap<int>(-8, 0, 10) == 0);
    expect(math_util::cap<int>(15, 0, 10) == 10);
    expect(math_util::cap<float>(20.5f, 0.0f, 30.0f) == 20.5f);
    expect(math_util::cap<float>(1.0f, 0.0f, 2.0f) == 1.0f);
    expect(math_util::cap<float>(-2.0f, -5.0f, 5.0f) == -2.0f);
    expect(math_util::cap<float>(1.0f, 0.0f, 0.0f) == 0);
  };

  "percentage"_test = [] {
    expect(math_util::percentage<float, float>(5.5f, 0.0f, 10.0f) == 55.0f);
    expect(math_util::percentage<float, int>(5.55f, 0.0f, 10.0f) == 56);
    expect(math_util::percentage<float, float>(5.25f, 0.0f, 12.0f) == 43.75f);
    expect(math_util::percentage<int, int>(5, 0, 12) == 41);
    expect(math_util::percentage<float, float>(20.5f, 0.0f, 100.0f) == 20.5f);
    expect(math_util::percentage<float, float>(4.5f, 1.0f, 6.0f) == 70.0f);
    expect(math_util::percentage<float, int>(20.5f, 0.0f, 100.0f) == 21);
    expect(math_util::percentage<int, int>(4, 2, 6) == 50);
    expect(math_util::percentage<int, int>(0, -10, 10) == 50);
    expect(math_util::percentage<int, int>(-10, -10, 10) == 0);
    expect(math_util::percentage<int, int>(10, -10, 10) == 100);
    expect(math_util::percentage(10, 0, 100) == 10);
  };

  "percentage_to_value"_test = [] {
    expect(math_util::percentage_to_value(50, 5) == 3);
    expect(math_util::percentage_to_value<int, float>(50, 5) == 2.5f);
    expect(math_util::percentage_to_value(0, 5) == 0);
    expect(math_util::percentage_to_value(10, 5) == 1);
    expect(math_util::percentage_to_value(20, 5) == 1);
    expect(math_util::percentage_to_value(30, 5) == 2);
    expect(math_util::percentage_to_value(40, 5) == 2);
    expect(math_util::percentage_to_value(50, 5) == 3);
    expect(math_util::percentage_to_value(100, 5) == 5);
    expect(math_util::percentage_to_value(200, 5) == 5);
    expect(math_util::percentage_to_value(-30, 5) == 0);
  };
}
