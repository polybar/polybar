#include "utils/units.hpp"

#include "common/test.hpp"
#include "utils/units.hpp"

using namespace polybar;
using namespace units_utils;

namespace polybar {
  bool operator==(const extent_val lhs, const extent_val rhs) {
    return lhs.type == rhs.type && lhs.value == rhs.value;
  }

  bool operator==(const spacing_val lhs, const spacing_val rhs) {
    return lhs.type == rhs.type && lhs.value == rhs.value;
  }
} // namespace polybar

/**
 * \brief Class for parameterized tests on geom_format_to_pixels
 *
 * The first element in the tuple is the expected return value, the second
 * value represents the format string. The max value is always 1000 and dpi is always 96
 */
class GeomFormatToPixelsTest : public ::testing::Test,
                               public ::testing::WithParamInterface<pair<int, percentage_with_offset>> {};

vector<pair<int, percentage_with_offset>> to_pixels_no_offset_list = {
    {1000, percentage_with_offset{100.}},
    {0, percentage_with_offset{0.}},
    {1000, percentage_with_offset{150.}},
    {100, percentage_with_offset{10.}},
    {0, percentage_with_offset{0., ZERO_PX_EXTENT}},
    {1234, percentage_with_offset{0., extent_val{extent_type::PIXEL, 1234}}},
    {1, percentage_with_offset{0., extent_val{extent_type::PIXEL, 1}}},
};

vector<pair<int, percentage_with_offset>> to_pixels_with_pixels_list = {
    {1000, percentage_with_offset{100., ZERO_PX_EXTENT}},
    {1010, percentage_with_offset{100., extent_val{extent_type::PIXEL, 10}}},
    {990, percentage_with_offset{100., extent_val{extent_type::PIXEL, -10}}},
    {10, percentage_with_offset{0., extent_val{extent_type::PIXEL, 10}}},
    {1000, percentage_with_offset{99., extent_val{extent_type::PIXEL, 10}}},
    {-90, percentage_with_offset{1., extent_val{extent_type::PIXEL, -100}}},
};

vector<pair<int, percentage_with_offset>> to_pixels_with_points_list = {
    {1013, percentage_with_offset{100., extent_val{extent_type::POINT, 10}}},
    {987, percentage_with_offset{100., extent_val{extent_type::POINT, -10}}},
    {1003, percentage_with_offset{99., extent_val{extent_type::POINT, 10}}},
    {13, percentage_with_offset{0., extent_val{extent_type::POINT, 10}}},
};

INSTANTIATE_TEST_SUITE_P(NoOffset, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_no_offset_list));

INSTANTIATE_TEST_SUITE_P(WithPixels, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_pixels_list));

INSTANTIATE_TEST_SUITE_P(WithPoints, GeomFormatToPixelsTest, ::testing::ValuesIn(to_pixels_with_points_list));

static constexpr int MAX_WIDTH = 1000;
static constexpr int DPI = 96;

TEST_P(GeomFormatToPixelsTest, correctness) {
  int exp = GetParam().first;
  percentage_with_offset geometry = GetParam().second;
  EXPECT_DOUBLE_EQ(exp, percentage_with_offset_to_pixel(geometry, MAX_WIDTH, DPI));
}

TEST(UnitsUtils, point_to_pixel) {
  EXPECT_EQ(72, point_to_pixel(72, 72));
  EXPECT_EQ(96, point_to_pixel(72, 96));
  EXPECT_EQ(48, point_to_pixel(36, 96));
  EXPECT_EQ(-48, point_to_pixel(-36, 96));
}

TEST(UnitsUtils, extent_to_pixel) {
  EXPECT_EQ(100, extent_to_pixel_nonnegative({extent_type::PIXEL, 100}, 0));
  EXPECT_EQ(48, extent_to_pixel_nonnegative({extent_type::POINT, 36}, 96));

  EXPECT_EQ(0, extent_to_pixel_nonnegative({extent_type::PIXEL, -100}, 0));
  EXPECT_EQ(0, extent_to_pixel_nonnegative({extent_type::POINT, -36}, 96));
}

TEST(UnitsUtils, parse_extent_unit) {
  EXPECT_EQ(extent_type::PIXEL, parse_extent_unit("px"));
  EXPECT_EQ(extent_type::POINT, parse_extent_unit("pt"));

  EXPECT_EQ(extent_type::PIXEL, parse_extent_unit(""));

  EXPECT_THROW(parse_extent_unit("foo"), std::runtime_error);
}

TEST(UnitsUtils, parse_extent) {
  EXPECT_EQ((extent_val{extent_type::PIXEL, 100}), parse_extent("100px"));
  EXPECT_EQ((extent_val{extent_type::POINT, 36}), parse_extent("36pt"));

  EXPECT_EQ((extent_val{extent_type::PIXEL, -100}), parse_extent("-100px"));
  EXPECT_EQ((extent_val{extent_type::POINT, -36}), parse_extent("-36pt"));

  EXPECT_EQ((extent_val{extent_type::PIXEL, 100}), parse_extent("100"));
  EXPECT_EQ((extent_val{extent_type::PIXEL, -100}), parse_extent("-100"));

  EXPECT_THROW(parse_extent("100foo"), std::runtime_error);
}

TEST(UnitsUtils, extent_to_string) {
  EXPECT_EQ("100px", extent_to_string({extent_type::PIXEL, 100}));
  EXPECT_EQ("36pt", extent_to_string({extent_type::POINT, 36}));

  EXPECT_EQ("-100px", extent_to_string({extent_type::PIXEL, -100}));
  EXPECT_EQ("-36pt", extent_to_string({extent_type::POINT, -36}));
}

TEST(UnitsUtils, parse_spacing_unit) {
  EXPECT_EQ(spacing_type::PIXEL, parse_spacing_unit("px"));
  EXPECT_EQ(spacing_type::POINT, parse_spacing_unit("pt"));

  EXPECT_EQ(spacing_type::SPACE, parse_spacing_unit(""));

  EXPECT_THROW(parse_spacing_unit("foo"), std::runtime_error);
}

TEST(UnitsUtils, parse_spacing) {
  EXPECT_EQ((spacing_val{spacing_type::PIXEL, 100}), parse_spacing("100px"));
  EXPECT_EQ((spacing_val{spacing_type::POINT, 36}), parse_spacing("36pt"));

  EXPECT_EQ((spacing_val{spacing_type::SPACE, 100}), parse_spacing("100"));

  EXPECT_THROW(parse_spacing("-100px"), std::runtime_error);
  EXPECT_THROW(parse_spacing("-36pt"), std::runtime_error);
  EXPECT_THROW(parse_spacing("-100"), std::runtime_error);
  EXPECT_THROW(parse_spacing("100foo"), std::runtime_error);
}
