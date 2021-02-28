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
                               public ::testing::WithParamInterface<pair<unsigned int, geometry_format_values>> {};

vector<pair<unsigned int, geometry_format_values>> to_pixels_no_offset_list = {
    {1000, geometry_format_values{100.}},
    {0, geometry_format_values{0.}},
    {1000, geometry_format_values{150.}},
    {100, geometry_format_values{10.}},
    {0, geometry_format_values{0., geometry{size_type::PIXEL, 0}}},
    {1234, geometry_format_values{0., geometry{size_type::PIXEL, 1234}}},
    {1, geometry_format_values{0., geometry{size_type::PIXEL, 1}}},
};

vector<pair<unsigned int, geometry_format_values>> to_pixels_with_offset_list = {
    {1000, geometry_format_values{100., geometry{size_type::PIXEL, 0}}},
    {1010, geometry_format_values{100., geometry{size_type::PIXEL, 10}}},
    {990, geometry_format_values{100., geometry{size_type::PIXEL, -10}}},
    {10, geometry_format_values{0., geometry{size_type::PIXEL, 10}}},
    {1000, geometry_format_values{99., geometry{size_type::PIXEL, 10}}},
    {0, geometry_format_values{1., geometry{size_type::PIXEL, -100}}},
};

vector<pair<unsigned int, geometry_format_values>> to_pixels_with_units_list = {
    {1013, geometry_format_values{100., geometry{size_type::POINT, 10}}},
    {987, geometry_format_values{100., geometry{size_type::POINT, -10}}},
    {1003, geometry_format_values{99., geometry{size_type::POINT, 10}}},
    {13, geometry_format_values{0., geometry{size_type::POINT, 10}}},
    {0, geometry_format_values{0, geometry{size_type::POINT, -10}}},
};

INSTANTIATE_TEST_SUITE_P(NoOffset, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_no_offset_list));

INSTANTIATE_TEST_SUITE_P(WithOffset, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_offset_list));

INSTANTIATE_TEST_SUITE_P(WithUnits, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_units_list));

TEST_P(GeomFormatToPixelsTest, correctness) {
  unsigned int exp = GetParam().first;
  polybar::geometry_format_values geometry = GetParam().second;
  EXPECT_DOUBLE_EQ(exp, geom_format_to_pixels(geometry, 1000, 96));
}
