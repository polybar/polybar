#include "common/test.hpp"
#include "common/config_test.hpp"
#include "drawtypes/labellist.hpp"
#include "utils/factory.hpp"

using namespace polybar;
using namespace polybar::drawtypes;

TEST(LabelList, load) {
  logger log(loglevel::NONE);
  string config_txt = "./test_config.ini";
  config_parser parser(log, move(config_txt), "example");
  config::make_type conf = parser.parse();
  vector<label_t> labels;
  label_t tmplate;
  load_labellist(labels, tmplate, conf, "test", "label", false);
  EXPECT_EQ(3, labels.size());
  EXPECT_EQ("A", get_text(labels[0]));
  EXPECT_EQ("B", get_text(labels[1]));
  EXPECT_EQ("C", get_text(labels[2]));
  EXPECT_NE(nullptr, tmplate);
  EXPECT_EQ("%label% %percentage%", get_text(tmplate));
}

