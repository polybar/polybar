#include "utils/math.hpp"

#include "common/test.hpp"

using namespace polybar;

TEST(Math, cap) {
  EXPECT_EQ(8, math_util::cap<int>(8, 0, 10));
  EXPECT_EQ(0, math_util::cap<int>(-8, 0, 10));
  EXPECT_EQ(10, math_util::cap<int>(15, 0, 10));
  EXPECT_EQ(20.5f, math_util::cap<float>(20.5f, 0.0f, 30.0f));
  EXPECT_EQ(1.0f, math_util::cap<float>(1.0f, 0.0f, 2.0f));
  EXPECT_EQ(-2.0f, math_util::cap<float>(-2.0f, -5.0f, 5.0f));
  EXPECT_EQ(0, math_util::cap<float>(1.0f, 0.0f, 0.0f));
}

TEST(Math, unbounded_percentage) {
  EXPECT_EQ(101.0f, (math_util::unbounded_percentage<float, float>(101.0f, 0.0f, 100.0f)));
  EXPECT_EQ(102, (math_util::unbounded_percentage<float, int>(101.5f, 0.0f, 100.0f)));
  EXPECT_EQ(110.0f, (math_util::unbounded_percentage<float, float>(12.0f, -10.0f, 10.0f)));
  EXPECT_EQ(150.0f, (math_util::unbounded_percentage<float, float>(11.5f, 10.0f, 11.0f)));
  EXPECT_EQ(-50.0f, (math_util::unbounded_percentage<float, float>(-50.0f, 0.0f, 100.0f)));
  EXPECT_EQ(-50.0f, (math_util::unbounded_percentage<float, float>(9.5f, 10.0f, 11.0f)));
}

TEST(Math, percentage) {
  EXPECT_EQ(55.0f, (math_util::percentage<float, float>(5.5f, 0.0f, 10.0f)));
  EXPECT_EQ(56, (math_util::percentage<float, int>(5.55f, 0.0f, 10.0f)));
  EXPECT_EQ(43.75f, (math_util::percentage<float, float>(5.25f, 0.0f, 12.0f)));
  EXPECT_EQ(42, (math_util::percentage<int, int>(5, 0, 12)));
  EXPECT_EQ(20.5f, (math_util::percentage<float, float>(20.5f, 0.0f, 100.0f)));
  EXPECT_EQ(70.0f, (math_util::percentage<float, float>(4.5f, 1.0f, 6.0f)));
  EXPECT_EQ(21, (math_util::percentage<float, int>(20.5f, 0.0f, 100.0f)));
  EXPECT_EQ(50, (math_util::percentage<int, int>(4, 2, 6)));
  EXPECT_EQ(50, (math_util::percentage<int, int>(0, -10, 10)));
  EXPECT_EQ(0, (math_util::percentage<int, int>(-10, -10, 10)));
  EXPECT_EQ(100, (math_util::percentage<int, int>(10, -10, 10)));
  EXPECT_EQ(10, (math_util::percentage(10, 0, 100)));
}

TEST(Math, percentageToValue) {
  EXPECT_EQ(3, math_util::percentage_to_value(50, 5));
  EXPECT_EQ(2.5f, (math_util::percentage_to_value<int, float>(50, 5)));
  EXPECT_EQ(0, math_util::percentage_to_value(0, 5));
  EXPECT_EQ(1, math_util::percentage_to_value(10, 5));
  EXPECT_EQ(1, math_util::percentage_to_value(20, 5));
  EXPECT_EQ(2, math_util::percentage_to_value(30, 5));
  EXPECT_EQ(2, math_util::percentage_to_value(40, 5));
  EXPECT_EQ(3, math_util::percentage_to_value(50, 5));
  EXPECT_EQ(5, math_util::percentage_to_value(100, 5));
  EXPECT_EQ(5, math_util::percentage_to_value(200, 5));
  EXPECT_EQ(0, math_util::percentage_to_value(-30, 5));
}

TEST(Math, rangedPercentageToValue) {
  EXPECT_EQ(250, math_util::percentage_to_value(50, 200, 300));
  EXPECT_EQ(3, math_util::percentage_to_value(50, 1, 5));
}

TEST(Math, roundToNearest10) {
  EXPECT_EQ(50, math_util::nearest_10(52));
  EXPECT_EQ(10, math_util::nearest_10(9.1));
  EXPECT_EQ(100, math_util::nearest_10(95.0));
  EXPECT_EQ(90, math_util::nearest_10(94.9));
}

TEST(Math, roundToNearest5) {
  EXPECT_EQ(50, math_util::nearest_5(52));
  EXPECT_EQ(10, math_util::nearest_5(9.1));
  EXPECT_EQ(95, math_util::nearest_5(95.0));
  EXPECT_EQ(95, math_util::nearest_5(94.9));
  EXPECT_EQ(0, math_util::nearest_5(1));
  EXPECT_EQ(100, math_util::nearest_5(99.99));
}
