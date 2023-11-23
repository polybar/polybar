#include "components/config.hpp"

#include <optional>

#include "cairo/utils.hpp"
#include "common/test.hpp"
#include "components/types.hpp"
#include "drawtypes/label.hpp"

using namespace polybar;
using namespace std;

class Config : public config {
 public:
  Config(std::string name, sectionmap_t sections = {}) : config(l, "/dev/zero", move(name)) {
    if (!sections.empty()) {
      set_sections(sections);
    } else {
      // clang-format off
      set_sections({
        {
          "settings", {
            {"compositing-border", "5"}
          }
        },
        {
          "bar/ut_bar", {
            {"modules-left", "unittest_name"},
            {"width", "100%"},
            {"height", "18"},
            {"fixed-center", "false"},
            {"list1-0", "VALUE0"},
            {"list1-1", "VALUE1"},
            {"list2-0", "VALUE0"},
            {"list3", "VALUE3"},
            {"list3-0-width", "100%"},
            {"list3-0-height", "18"},
            {"list3-0-fixed-center", "false"},
            {"list3-0-list-0-width", "100%"},
            {"list3-0-list-0-height", "18"},
            {"list3-0-list-0-fixed-center", "false"},
            {"list3-0-list-1-width", "30%"},
            {"list3-0-list-1-height", "8"},
            {"list3-0-list-1-fixed-center", "true"},
            {"bool1", "true"},
            {"bool2", "yes"},
            {"bool3", "on"},
            {"bool4", "1"},
            {"bool5", "tRuE"},
            {"bool6", "unhandled value means false"},
            {"float", "1.234567891"},
            {"spacing", "100px"},
            {"percent", "36.5%:42.7pt"},
            {"color", "#abc"},
            {"operator", "over"}
          }
        },
        {
          "module/unittest_name", {
            {"type", "internal/unittest"},
            {"list-0", "12"},
            {"list-1", "13"},
            {"list-2", "14"},
            {"list-3", "15"}
          }
        },
        {
          "module/my_script", {
            {"type", "internal/script"},
            {"env-VAR1", "VALUE1"},
            {"env-VAR2", "VALUE2"},
            {"env-VAR3", "VALUE3"},
            {"env2-VAR1", "VALUE1"}
          }
        }
      });
      // clang-format on
    }
  }

  const logger l = logger(loglevel::NONE);

  bool has(const string& section, const string& key) const {
    return config::has(section, key);
  }

  template <typename T = string>
  T setting_get(const string& key, const T& default_value) const {
    return config::setting_get<T>(key, default_value);
  }

  template <typename T = string>
  T get(const string& section, const string& key) const {
    return config::get<T>(section, key);
  }

  template <typename T = string>
  T get(const string& section, const string& key, const T& default_value) const {
    return config::get<T>(section, key, default_value);
  }

  bool bar_has(const string& key) const {
    return config::bar_has(key);
  }

  template <typename T = string>
  T bar_get(const string& key) const {
    return config::bar_get<T>(key);
  }
  template <typename T = string>
  vector<T> bar_get_list(const string& key) const {
    return config::bar_get_list<T>(key);
  }
  template <typename T = string>
  vector<T> bar_get_list(const string& key, const vector<T>& default_value) const {
    return config::bar_get_list<T>(key, default_value);
  }
  template <typename T = string>
  T bar_deprecated(const string& old, const string& newkey, const T& fallback) const {
    return config::bar_deprecated<T>(old, newkey, fallback);
  }
};

/**
 * \brief Fixture class
 */
class ConfigTest : public Config, public ::testing::Test {
 public:
  ConfigTest(std::string name) : Config(name), ::testing::Test() {}
};

class HasTest : public ConfigTest, public ::testing::WithParamInterface<pair<pair<optional<string>, string>, bool>> {
 public:
  HasTest() : ConfigTest("ut_bar") {}
};

// clang-format off
vector<pair<pair<optional<string>, string>, bool>> has_test_input = {
  {{nullopt, "modules-left"}, true},
  {{"module/unittest_name", "type"}, true},
  {{"module/UnitTestName", "type"}, false},
  {{"module/unittest_name", "TYPE"}, false}
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Inst, HasTest, ::testing::ValuesIn(has_test_input));

TEST_P(HasTest, correctness) {
  if (GetParam().first.first) {
    EXPECT_EQ(this->has(GetParam().first.first.value(), GetParam().first.second), GetParam().second);
  } else {
    EXPECT_EQ(this->bar_has(GetParam().first.second), GetParam().second);
  }
}

class BarGet : public ConfigTest, public ::testing::WithParamInterface<pair<string, string>> {
 public:
  BarGet() : ConfigTest("ut_bar") {}
};

// clang-format off
vector<pair<string, string>> bar_get_input = {
  {"modules-left", "unittest_name"},
  {"width", "100%"},
  {"height", "18"},
  {"fixed-center", "false"}
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Inst, BarGet, ::testing::ValuesIn(bar_get_input));

TEST_P(BarGet, correctness) {
  EXPECT_EQ(this->bar_get(GetParam().first), GetParam().second);
  EXPECT_THROW(this->bar_get(GetParam().first + "___"), key_error);
}

TEST(SettingsGet, correctness) {
  Config c("ut_bar");
  string def{"default_value"};
  EXPECT_EQ(c.setting_get("compositing-border", def), "5");
  EXPECT_EQ(c.setting_get("compositing-border___", def), def);
}

TEST(ModuleGet, correctness) {
  Config c("ut_bar");
  EXPECT_EQ(c.get("module/unittest_name", "type"), "internal/unittest");
  EXPECT_THROW(c.get("module/unittest_name___", "type"), key_error);
  EXPECT_THROW(c.get("module/unittest_name", "type___"), key_error);
  EXPECT_EQ(c.get("module/unittest_name", "type", string("default_value")), "internal/unittest");
  EXPECT_EQ(c.get("module/unittest_name___", "type", string("default_value")), "default_value");
  EXPECT_EQ(c.get("module/unittest_name", "type___", string("default_value")), "default_value");
}

class GetWithPrefix : public ConfigTest,
                      public ::testing::WithParamInterface<pair<pair<string, string>, map<string, string>>> {
 public:
  GetWithPrefix() : ConfigTest("ut_bar") {}
};

// clang-format off
vector<pair<pair<string, string>, map<string, string>>> get_with_prefix_input = {
  {{"module/my_script", "env-"}, {{"VAR1", "VALUE1"}, {"VAR2", "VALUE2"}, {"VAR3", "VALUE3"}}},
  {{"module/my_script", "env2-"}, {{"VAR1", "VALUE1"}}}
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Inst, GetWithPrefix, ::testing::ValuesIn(get_with_prefix_input));

TEST_P(GetWithPrefix, found) {
  unordered_map<string, string> res = this->get_with_prefix(GetParam().first.first, GetParam().first.second);
  for (const auto& p : GetParam().second) {
    EXPECT_NE(std::find(res.begin(), res.end(), p), res.end());
  }
}

TEST_P(GetWithPrefix, missingSection) {
  EXPECT_THROW(this->get_with_prefix(GetParam().first.first + "___", GetParam().first.second), key_error);
}

TEST_P(GetWithPrefix, missingKey) {
  EXPECT_TRUE(this->get_with_prefix(GetParam().first.first, string("EE") + GetParam().first.second).empty());
}

class GetList : public ConfigTest, public ::testing::WithParamInterface<pair<string, vector<string>>> {
 public:
  GetList() : ConfigTest("ut_bar") {}
};

// clang-format off
vector<pair<string, vector<string>>> get_list_input = {
  {"list1", {"VALUE0", "VALUE1"}},
  {"list2", {"VALUE0"}}
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(Inst, GetList, ::testing::ValuesIn(get_list_input));

TEST_P(GetList, found) {
  EXPECT_EQ(this->bar_get_list(GetParam().first), GetParam().second);
}

TEST_P(GetList, foundWithSection) {
  EXPECT_EQ(this->bar_get_list(GetParam().first), GetParam().second);
}

TEST_P(GetList, missingSection) {
  EXPECT_THROW(this->get_list("unknown", GetParam().first), key_error);
}

TEST_P(GetList, missingKeyWithSection) {
  EXPECT_THROW(this->bar_get_list(GetParam().first + "___"), key_error);
}

TEST_P(GetList, foundWithSectionAndDefault) {
  EXPECT_EQ(this->bar_get_list<string>(GetParam().first, {"def1", "def2"}), GetParam().second);
}

TEST_P(GetList, missingSectionWithDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(this->get_list<string>("unknown", GetParam().first, def), def);
}

TEST_P(GetList, missingKeyWithSectionAndDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(this->bar_get_list<string>(GetParam().first + "___", def), def);
}

TEST_F(BarGet, deprecated) {
  string def{"default_value"};
  EXPECT_EQ(this->bar_deprecated("width", "unused", def), "100%");
  EXPECT_EQ(this->bar_deprecated("unknown", "width", def), "100%");
  EXPECT_EQ(this->bar_deprecated("unknown", "unknown2", def), def);
}

TEST_F(BarGet, typed) {
  EXPECT_EQ(this->bar_get<string>("height"), string{"18"});
  EXPECT_EQ(this->bar_get<char>("height"), '1');
  EXPECT_EQ(this->bar_get<int>("height"), 18);
  EXPECT_EQ(this->bar_get<short>("height"), (short)18);
  EXPECT_EQ(this->bar_get<long>("height"), 18L);
  EXPECT_EQ(this->bar_get<long long>("height"), 18LL);
  EXPECT_EQ(this->bar_get<unsigned char>("height"), (unsigned char)18);
  EXPECT_EQ(this->bar_get<unsigned short>("height"), (unsigned short)18);
  EXPECT_EQ(this->bar_get<unsigned int>("height"), (unsigned int)18);
  EXPECT_EQ(this->bar_get<unsigned long>("height"), (unsigned long)18);
  EXPECT_EQ(this->bar_get<unsigned long long>("height"), (unsigned long long)18);
  EXPECT_EQ(this->bar_get<unsigned short>("height"), (unsigned short)18);
  EXPECT_EQ(this->bar_get<unsigned short>("height"), (unsigned short)18);

  EXPECT_EQ(this->bar_get<bool>("bool1"), true);
  EXPECT_EQ(this->bar_get<bool>("bool2"), true);
  EXPECT_EQ(this->bar_get<bool>("bool3"), true);
  EXPECT_EQ(this->bar_get<bool>("bool4"), true);
  EXPECT_EQ(this->bar_get<bool>("bool5"), true);
  EXPECT_EQ(this->bar_get<bool>("bool6"), false);

  EXPECT_FLOAT_EQ(1.234567891f, 1.23456788);
  EXPECT_FLOAT_EQ(this->bar_get<float>("float"), 1.23456788);
  EXPECT_DOUBLE_EQ(this->bar_get<double>("float"), 1.234567891);

  spacing_val sp_val{this->bar_get<spacing_val>("spacing")};
  EXPECT_EQ(sp_val.type, spacing_type::PIXEL);
  EXPECT_EQ(sp_val.value, 100);

  extent_val ex_val{this->bar_get<extent_val>("spacing")};
  EXPECT_EQ(ex_val.type, extent_type::PIXEL);
  EXPECT_EQ(ex_val.value, 100);

  percentage_with_offset p1{this->bar_get<percentage_with_offset>("width")};
  EXPECT_DOUBLE_EQ(p1.percentage, 100.);
  EXPECT_EQ(p1.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p1.offset.value, 0.f);

  percentage_with_offset p2{this->bar_get<percentage_with_offset>("height")};
  EXPECT_DOUBLE_EQ(p2.percentage, 0.);
  EXPECT_EQ(p2.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p2.offset.value, 18.f);

  percentage_with_offset p3{this->bar_get<percentage_with_offset>("percent")};
  EXPECT_DOUBLE_EQ(p3.percentage, 36.5);
  EXPECT_EQ(p3.offset.type, extent_type::POINT);
  EXPECT_FLOAT_EQ(p3.offset.value, 42.7f);

  EXPECT_EQ(this->bar_get<chrono::seconds>("height"), chrono::seconds{18});
  EXPECT_EQ(this->bar_get<chrono::milliseconds>("height"), chrono::milliseconds{18});
  EXPECT_EQ(this->bar_get<chrono::duration<double>>("float"), chrono::duration<double>{1.234567891});

  EXPECT_EQ(this->bar_get<rgba>("color"), rgba{"#abc"});

  EXPECT_EQ(this->bar_get<cairo_operator_t>("operator"), cairo_operator_t{CAIRO_OPERATOR_OVER});

  // Cannot test bar_get<const char *> because address sanitizer throws stack-use-after-return
  // for any access to height_chr
  // const char *height_chr = this->bar_get<const char *>("height");
  // EXPECT_EQ(strlen(height_chr), 2);
  // EXPECT_EQ(strncmp(height_chr, "18", 2), 0);
}

TEST(BadConfig, MissingBarName) {
  Config c("Ut_Bar");
  EXPECT_FALSE(c.bar_has("modules-left"));
  EXPECT_THROW(c.bar_get("foreground"), key_error);
  EXPECT_THROW(c.bar_get_list("list"), key_error);
  string def{"default_value"};
  EXPECT_EQ(c.bar_deprecated("width", "unused", def), def);
}

TEST_F(BarGet, OperatorAccess) {
  config::value bars_accessor = this->operator[](config::value::BARS_ENTRY);
  config::value bar_accessor = bars_accessor[this->bar_name()];
  config::value settings_accessor = this->operator[](config::value::SETTINGS_ENTRY);
  config::value modules_accessor = this->operator[](config::value::MODULES_ENTRY);
  // Ok cases
  EXPECT_EQ(bar_accessor["width"].as<string>(), "100%");
  EXPECT_EQ(bar_accessor["height"].as<int>(), 18);
  EXPECT_EQ(bar_accessor["fixed-center"].as<bool>(), false);
  EXPECT_EQ(bar_accessor["list3"].as<string>(), "VALUE3");
  EXPECT_EQ(bar_accessor["list3"][0]["width"].as<string>(), "100%");
  EXPECT_EQ(bar_accessor["list3"][0]["height"].as<int>(), 18);
  EXPECT_EQ(bar_accessor["list3"][0]["fixed-center"].as<bool>(), false);
  EXPECT_EQ(bar_accessor["list3"][0]["list"][0]["width"].as<string>(), "100%");
  EXPECT_EQ(bar_accessor["list3"][0]["list"][0]["height"].as<int>(), 18);
  EXPECT_EQ(bar_accessor["list3"][0]["list"][0]["fixed-center"].as<bool>(), false);
  EXPECT_EQ(bar_accessor["list3"][0]["list"][1]["width"].as<string>(), "30%");
  EXPECT_EQ(bar_accessor["list3"][0]["list"][1]["height"].as<int>(), 8);
  EXPECT_EQ(bar_accessor["list3"][0]["list"][1]["fixed-center"].as<bool>(), true);
  EXPECT_EQ(settings_accessor["compositing-border"].as<int>(8), 5);
  EXPECT_EQ(modules_accessor["unittest_name"]["type"].as<string>(), "internal/unittest");
  EXPECT_EQ(modules_accessor["my_script"]["type"].as<string>(), "internal/script");
  EXPECT_EQ(modules_accessor["my_script"]["env-VAR1"].as<string>(), "VALUE1");
  EXPECT_EQ(modules_accessor["my_script"]["env"]["VAR1"].as<string>(), "VALUE1");

  // Bad Access cases
  EXPECT_THROW(this->operator[]("barS")[this->bar_name()]["width"].as<string>(), key_error);
  EXPECT_THROW(this->operator[]("bars")["unknown_bar"]["width"].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["wiDth"].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["list4"][0]["width"].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["list4"][0]["height"].as<int>(), key_error);
  EXPECT_THROW(bar_accessor["list4"][0]["fixed-center"].as<bool>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][1]["width"].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][1]["height"].as<int>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][1]["fixed-center"].as<bool>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][0]["__width"].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][0]["__height"].as<int>(), key_error);
  EXPECT_THROW(bar_accessor["list3"][0]["__fixed-center"].as<bool>(), key_error);
  EXPECT_THROW(settings_accessor["missing_key"].as<int>(), key_error);
  EXPECT_THROW(settings_accessor[0].as<int>(8), runtime_error);
  EXPECT_THROW(bar_accessor["width"][0].as<string>(), key_error);
  EXPECT_THROW(bar_accessor["width"]["wrong"].as<string>(), key_error);

  // Bad Access cases with default value
  EXPECT_EQ(bar_accessor["wiDth"].as<string>("default"), "default");
  EXPECT_EQ(bar_accessor["list4"][0]["width"].as<string>("default"), "default");
  EXPECT_EQ(bar_accessor["list4"][0]["height"].as<int>(42), 42);
  EXPECT_EQ(bar_accessor["list4"][0]["fixed-center"].as<bool>(true), true);
  EXPECT_EQ(bar_accessor["list3"][1]["width"].as<string>("default"), "default");
  EXPECT_EQ(bar_accessor["list3"][1]["height"].as<int>(421), 421);
  EXPECT_EQ(bar_accessor["list3"][1]["fixed-center"].as<bool>(false), false);
  EXPECT_EQ(bar_accessor["list3"][0]["__width"].as<string>("default"), "default");
  EXPECT_EQ(bar_accessor["list3"][0]["__height"].as<int>(32), 32);
  EXPECT_EQ(bar_accessor["list3"][0]["__fixed-center"].as<bool>(true), true);
  EXPECT_EQ(settings_accessor["missing_key"].as<int>(321), 321);
  EXPECT_EQ(bar_accessor["width"][0].as<string>("default"), "default");
  EXPECT_EQ(bar_accessor["width"]["wrong"].as<string>("default"), "default");

  // Has - First level
  EXPECT_TRUE(bars_accessor.has(this->bar_name()));
  EXPECT_FALSE(bars_accessor.has("another_bar"));
  EXPECT_TRUE(settings_accessor.has("compositing-border"));
  EXPECT_FALSE(settings_accessor.has("Compositing-border"));
  EXPECT_TRUE(modules_accessor.has("my_script"));
  EXPECT_FALSE(modules_accessor.has("My_script"));

  // Has - Subkeys
  EXPECT_TRUE(bar_accessor.has("width"));
  EXPECT_TRUE(modules_accessor["my_script"].has("type"));
  EXPECT_FALSE(bar_accessor.has("list8"));

  // Has - list
  EXPECT_TRUE(bar_accessor.has("list1"));
  EXPECT_FALSE(settings_accessor.has("compositing"));

  // Bad Cast cases
  EXPECT_THROW(bar_accessor["fixed-center"].as<spacing_val>(), invalid_argument);
}

TEST(ConfigLabel, LoadLabel) {
  // clang-format off
  Config c("Ut_Bar", {
    {
      "module/my-text-label",
      {
        {"label-mounted", "%{F#0a81f5}Home%{F-}: %percentage_used%%"},
        {"label-mounted-foreground", "#000000ff"},
        {"label-mounted-background", "#00ff0000"},
        {"label-mounted-padding", "12pt"},
        {"label-mounted-margin", "42px"},
        {"label-unmounted", "%mountpoint% not mounted"},
        {"label-unmounted-foreground", "${colors.foreground-alt}"},

        {"label-NAME", "foobar"},
        {"label-NAME-foreground", "#00aa0000"},
        {"label-NAME-background", "#0000bb00"},
        {"label-NAME-overline", "#000000cc"},
        {"label-NAME-underline", "#ffaabbcc"},
        {"label-NAME-font", "4"},

        // Add N spaces, points, pixels before and after the label
        // Spacing added this way is not affected by the label colors
        // Default: 0
        {"label-NAME-padding-left", "12pt"},
        {"label-NAME-padding-right", "12px"},

        // Add N spaces, points, pixels before and after the label text
        // Spacing added this way is affected by the label colors
        // Default: 0
        {"label-NAME-margin-left", "42px"},
        {"label-NAME-margin-right", "42pt"},

        // Truncate text if it exceeds given limit. 
        // Default: 0
        {"label-NAME-maxlen", "30"},

        // Pad with spaces if the text doesn't have at least minlen characters
        // Default: 0
        {"label-NAME-minlen", "10"},
        // Alignment when the text is shorter than minlen
        // Possible Values: left, center, right
        // Default: left
        {"label-NAME-alignment", "center"},

        // Optionally append ... to the truncated string.
        // Default: true
        {"label-NAME-ellipsis", "false"},
      }
    }
  });
  // clang-format on
  config::value value = c[config::value::MODULES_ENTRY]["my-text-label"]["label-mounted"];
  label_t mounted = drawtypes::load_label(value);
  EXPECT_EQ(mounted->m_foreground, rgba{"#000000ff"});
  EXPECT_EQ(mounted->m_background, rgba{"#00ff0000"});
  EXPECT_EQ(mounted->m_padding.left.type, spacing_type::POINT);
  EXPECT_EQ(mounted->m_padding.left.value, 12);
  EXPECT_EQ(mounted->m_padding.left.type, spacing_type::POINT);
  EXPECT_EQ(mounted->m_padding.right.value, 12);
  EXPECT_EQ(mounted->m_margin.left.type, spacing_type::PIXEL);
  EXPECT_EQ(mounted->m_margin.left.value, 42);
  EXPECT_EQ(mounted->m_margin.left.type, spacing_type::PIXEL);
  EXPECT_EQ(mounted->m_margin.right.value, 42);

  config::value value2 = c[config::value::MODULES_ENTRY]["my-text-label"]["label-NAME"];
  label_t name = drawtypes::load_label(value2);
  EXPECT_EQ(name->m_foreground, rgba{"#00aa0000"});
  EXPECT_EQ(name->m_background, rgba{"#0000bb00"});
  EXPECT_EQ(name->m_overline, rgba{"#000000cc"});
  EXPECT_EQ(name->m_underline, rgba{"#ffaabbcc"});
  EXPECT_EQ(name->m_padding.left.type, spacing_type::POINT);
  EXPECT_EQ(name->m_padding.left.value, 12);
  EXPECT_EQ(name->m_padding.right.type, spacing_type::PIXEL);
  EXPECT_EQ(name->m_padding.right.value, 12);
  EXPECT_EQ(name->m_margin.left.type, spacing_type::PIXEL);
  EXPECT_EQ(name->m_margin.left.value, 42);
  EXPECT_EQ(name->m_margin.right.type, spacing_type::POINT);
  EXPECT_EQ(name->m_margin.right.value, 42);
}