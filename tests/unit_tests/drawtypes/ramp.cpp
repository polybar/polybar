#include "drawtypes/ramp.hpp"

#include "common/test.hpp"
#include "utils/factory.hpp"

using namespace polybar::drawtypes;
using namespace polybar;

TEST(Ramp, perc) {
  ramp r;
  r.add(std::make_shared<label>("test1", 0));
  r.add(std::make_shared<label>("test2", 0));
  r.add(std::make_shared<label>("test3", 0));
  EXPECT_EQ("test1", r.get_by_percentage(33)->get());
  EXPECT_EQ("test2", r.get_by_percentage(34)->get());
  EXPECT_EQ("test3", r.get_by_percentage(67)->get());
  EXPECT_EQ("test1", r.get_by_percentage_with_borders(19, 20, 40)->get());
  EXPECT_EQ("test2", r.get_by_percentage_with_borders(21, 20, 40)->get());
  EXPECT_EQ("test2", r.get_by_percentage_with_borders(39, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(41, 20, 40)->get());
  EXPECT_EQ("test1", r.get_by_percentage_with_borders(20, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(40, 20, 40)->get());
  r.add(std::make_shared<label>("test4", 0));
  EXPECT_EQ("test2", r.get_by_percentage_with_borders(29, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(31, 20, 40)->get());
}

TEST(Ramp, weights) {
  ramp r;
  r.add(std::make_shared<label>("test1", 0), 1);
  r.add(std::make_shared<label>("test2", 0), 2);
  r.add(std::make_shared<label>("test3", 0), 5);

  EXPECT_EQ("test1", r.get_by_percentage(12)->get());
  EXPECT_EQ("test2", r.get_by_percentage(13)->get());
  EXPECT_EQ("test2", r.get_by_percentage(37)->get());
  EXPECT_EQ("test3", r.get_by_percentage(38)->get());

  EXPECT_EQ("test1", r.get_by_percentage_with_borders(19, 20, 40)->get());
  EXPECT_EQ("test2", r.get_by_percentage_with_borders(21, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(39, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(41, 20, 40)->get());
  EXPECT_EQ("test1", r.get_by_percentage_with_borders(20, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(40, 20, 40)->get());
  r.add(std::make_shared<label>("test4", 0));
  r.add(std::make_shared<label>("test5", 0));
  EXPECT_EQ("test2", r.get_by_percentage_with_borders(24, 20, 40)->get());
  EXPECT_EQ("test3", r.get_by_percentage_with_borders(25, 20, 40)->get());
}
