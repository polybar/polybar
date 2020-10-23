#include "common/test.hpp"
#include "common/config_test.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/factory.hpp"

using namespace polybar::drawtypes;
using namespace polybar;

TEST(Ramp, perc) {
  logger log(loglevel::NONE);
  string config_txt = "./test_config.ini";
  config_parser parser(log, move(config_txt), "example");
  config::make_type conf = parser.parse();
  auto r = load_ramp(conf, "test", "label", false);
  r->get_template()->replace_token("%percentage%", "100");
  r->apply_template();
  EXPECT_EQ("A 100%", r->get_by_percentage(33)->get());
  EXPECT_EQ("B 100%", r->get_by_percentage(34)->get());
  EXPECT_EQ("C 100%", r->get_by_percentage(67)->get());
  EXPECT_EQ("A 100%", r->get_by_percentage_with_borders(19, 20, 40)->get());
  EXPECT_EQ("B 100%", r->get_by_percentage_with_borders(21, 20, 40)->get());
  EXPECT_EQ("B 100%", r->get_by_percentage_with_borders(39, 20, 40)->get());
  EXPECT_EQ("C 100%", r->get_by_percentage_with_borders(41, 20, 40)->get());

}
