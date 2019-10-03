#include "common/test.hpp"
#include "components/bar.hpp"

using namespace polybar;



/**
 * \brief Class for parameterized tests on geom_format_to_pixels
 *
 * The first element in the tuple is the expected return value, the second
 * value is the format string. The max value is always 1000
 */
class GeomFormatToPixelsTest :
  public ::testing::Test,
  public ::testing::WithParamInterface<pair<double, string>> {};

vector<pair<double, string>> to_pixels_no_offset_list = {
  {1000, "100%"},
  {0, "0%"},
  {1000, "150%"},
  {100, "10%"},
  {0, "0"},
  {1234, "1234"},
  {1.234, "1.234"},
};

vector<pair<double, string>> to_pixels_with_offset_list = {
  {1000, "100%:-0"},
  {1000, "100%:+0"},
  {1010, "100%:+10"},
  {990, "100%:-10"},
  {10, "0%:+10"},
  {1000, "99%:+10"},
  {0, "1%:-100"},
};

INSTANTIATE_TEST_SUITE_P(NoOffset, GeomFormatToPixelsTest,
    ::testing::ValuesIn(to_pixels_no_offset_list));

INSTANTIATE_TEST_SUITE_P(WithOffset, GeomFormatToPixelsTest,
    ::testing::ValuesIn(to_pixels_with_offset_list));

TEST_P(GeomFormatToPixelsTest, correctness) {
  double exp = GetParam().first;
  std::string str = GetParam().second;
  EXPECT_DOUBLE_EQ(exp, geom_format_to_pixels(str, 1000));
}
