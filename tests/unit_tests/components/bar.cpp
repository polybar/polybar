#include "components/bar.hpp"

#include "common/test.hpp"

using namespace polybar;

/**
 * \brief Class for parameterized tests on geom_format_to_pixels
 *
 * The first element in the tuple is the expected return value, the second
 * value is the format string. The max value is always 1000
 */
class GeomFormatToPixelsTest : public ::testing::Test,
                               public ::testing::WithParamInterface<pair<unsigned int, percentage_with_offset>> {};

vector<pair<unsigned int, percentage_with_offset>> to_pixels_no_offset_list = {
    {1000, percentage_with_offset{100.}},
    {0, percentage_with_offset{0.}},
    {1000, percentage_with_offset{150.}},
    {100, percentage_with_offset{10.}},
    {0, percentage_with_offset{0., ZERO_PX_EXTENT}},
    {1234, percentage_with_offset{0., extent_val{extent_type::PIXEL, 1234}}},
    {1, percentage_with_offset{0., extent_val{extent_type::PIXEL, 1}}},
};

vector<pair<unsigned int, percentage_with_offset>> to_pixels_with_offset_list = {
    {1000, percentage_with_offset{100., ZERO_PX_EXTENT}},
    {1010, percentage_with_offset{100., extent_val{extent_type::PIXEL, 10}}},
    {990, percentage_with_offset{100., extent_val{extent_type::PIXEL, -10}}},
    {10, percentage_with_offset{0., extent_val{extent_type::PIXEL, 10}}},
    {1000, percentage_with_offset{99., extent_val{extent_type::PIXEL, 10}}},
    {0, percentage_with_offset{1., extent_val{extent_type::PIXEL, -100}}},
};

vector<pair<unsigned int, percentage_with_offset>> to_pixels_with_units_list = {
    {1013, percentage_with_offset{100., extent_val{extent_type::POINT, 10}}},
    {987, percentage_with_offset{100., extent_val{extent_type::POINT, -10}}},
    {1003, percentage_with_offset{99., extent_val{extent_type::POINT, 10}}},
    {13, percentage_with_offset{0., extent_val{extent_type::POINT, 10}}},
    {0, percentage_with_offset{0, extent_val{extent_type::POINT, -10}}},
};

INSTANTIATE_TEST_SUITE_P(NoOffset, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_no_offset_list));

INSTANTIATE_TEST_SUITE_P(WithOffset, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_offset_list));

INSTANTIATE_TEST_SUITE_P(WithUnits, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_units_list));

TEST_P(GeomFormatToPixelsTest, correctness) {
  unsigned int exp = GetParam().first;
  polybar::percentage_with_offset geometry = GetParam().second;
  EXPECT_DOUBLE_EQ(exp, percentage_with_offset_to_pixel(geometry, 1000, 96));
}
