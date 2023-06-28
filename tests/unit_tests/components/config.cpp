#include "components/config.hpp"
#include "cairo/utils.hpp"

#include "common/test.hpp"

#include <optional>

using namespace polybar;
using namespace std;


class Config {
 public:
  Config(std::string name) {
    m_conf = make_unique<config>(l, "/dev/zero", move(name));
    m_conf->set_sections({
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
        "modules/unittest_name", {
          {"type", "internal/unittest"},
          {"list-0", "12"},
          {"list-1", "13"},
          {"list-2", "14"},
          {"list-3", "15"}
        }
      },
      {
        "modules/my_script", {
          {"type", "internal/script"},
          {"env-VAR1", "VALUE1"},
          {"env-VAR2", "VALUE2"},
          {"env-VAR3", "VALUE3"},
          {"env2-VAR1", "VALUE1"}
        }
      }
    });
  }

  const logger l = logger(loglevel::NONE);
  unique_ptr<config> m_conf;
};



/**
 * \brief Fixture class
 */
class ConfigTest : public Config, public ::testing::Test {
 public:
  ConfigTest(std::string name): Config(name), ::testing::Test() {}
};

class HasTest : public ConfigTest, public ::testing::WithParamInterface<pair<pair<optional<string>, string>, bool>> {
 public:
  HasTest(): ConfigTest("ut_bar"){}
};

vector<pair<pair<optional<string>, string>, bool>> has_test_input = {
  {{nullopt, "modules-left"}, true},
  {{"modules/unittest_name", "type"}, true},
  {{"modules/UnitTestName", "type"}, false},
  {{"modules/unittest_name", "TYPE"}, false}
};

INSTANTIATE_TEST_SUITE_P(Inst, HasTest, ::testing::ValuesIn(has_test_input));

TEST_P(HasTest, correctness) {
  if (GetParam().first.first) {
    EXPECT_EQ(m_conf->has(GetParam().first.first.value(), GetParam().first.second), GetParam().second);
  } else {
    EXPECT_EQ(m_conf->bar_has(GetParam().first.second), GetParam().second);
  }
}

class BarGet : public ConfigTest, public ::testing::WithParamInterface<pair<string, string>> {
 public:
  BarGet(): ConfigTest("ut_bar"){}
};

vector<pair<string, string>> bar_get_input = {
  {"modules-left", "unittest_name"},
  {"width", "100%"},
  {"height", "18"},
  {"fixed-center", "false"}
};

INSTANTIATE_TEST_SUITE_P(Inst, BarGet, ::testing::ValuesIn(bar_get_input));

TEST_P(BarGet, correctness) {
  EXPECT_EQ(m_conf->bar_get(GetParam().first), GetParam().second);
  EXPECT_THROW(m_conf->bar_get(GetParam().first + "___"), key_error);
}

TEST(SettingsGet, correctness) {
  Config c("ut_bar");
  string def{"default_value"};
  EXPECT_EQ(c.m_conf->setting_get("compositing-border", def), "5");
  EXPECT_EQ(c.m_conf->setting_get("compositing-border___", def), def);
}

TEST(ModuleGet, correctness) {
  Config c("ut_bar");
  EXPECT_EQ(c.m_conf->get("modules/unittest_name", "type"), "internal/unittest");
  EXPECT_THROW(c.m_conf->get("modules/unittest_name___", "type"), key_error);
  EXPECT_THROW(c.m_conf->get("modules/unittest_name", "type___"), key_error);
  EXPECT_EQ(c.m_conf->get("modules/unittest_name", "type", string("default_value")), "internal/unittest");
  EXPECT_EQ(c.m_conf->get("modules/unittest_name___", "type", string("default_value")), "default_value");
  EXPECT_EQ(c.m_conf->get("modules/unittest_name", "type___", string("default_value")), "default_value");
}

class GetWithPrefix : public ConfigTest, public ::testing::WithParamInterface<pair<pair<string, string>, vector<pair<string, string>>>> {
 public:
  GetWithPrefix(): ConfigTest("ut_bar"){}
};

vector<pair<pair<string, string>, vector<pair<string, string>>>> get_with_prefix_input = {
  {{"modules/my_script", "env-"}, {{"VAR1", "VALUE1"}, {"VAR2", "VALUE2"}, {"VAR3", "VALUE3"}}},
  {{"modules/my_script", "env2-"}, {{"VAR1", "VALUE1"}}}
};

INSTANTIATE_TEST_SUITE_P(Inst, GetWithPrefix, ::testing::ValuesIn(get_with_prefix_input));

TEST_P(GetWithPrefix, found) {
  vector<pair<string, string>> res = m_conf->get_with_prefix(GetParam().first.first, GetParam().first.second);
  for (const auto& p:GetParam().second) {
    EXPECT_NE(std::find(res.begin(), res.end(), p), res.end());

  }
}

TEST_P(GetWithPrefix, missingSection) {
  EXPECT_THROW(m_conf->get_with_prefix(GetParam().first.first + "___", GetParam().first.second), key_error);
}

TEST_P(GetWithPrefix, missingKey) {
  EXPECT_TRUE(m_conf->get_with_prefix(GetParam().first.first, string("EE") + GetParam().first.second).empty());
}

class GetList : public ConfigTest, public ::testing::WithParamInterface<pair<string, vector<string>>> {
 public:
  GetList(): ConfigTest("ut_bar"){}
};

vector<pair<string, vector<string>>> get_list_input = {
  {"list1", {"VALUE0", "VALUE1"}},
  {"list2", {"VALUE0"}}
};

INSTANTIATE_TEST_SUITE_P(Inst, GetList, ::testing::ValuesIn(get_list_input));

TEST_P(GetList, found) {
  EXPECT_EQ(m_conf->bar_get_list(GetParam().first), GetParam().second);
}

TEST_P(GetList, foundWithSection) {
  EXPECT_EQ(m_conf->bar_get_list(GetParam().first), GetParam().second);
}

TEST_P(GetList, missingSection) {
  EXPECT_THROW(m_conf->get_list("unknown", GetParam().first), key_error);
}

TEST_P(GetList, missingKeyWithSection) {
  EXPECT_THROW(m_conf->bar_get_list(GetParam().first + "___"), key_error);
}

TEST_P(GetList, foundWithSectionAndDefault) {
  EXPECT_EQ(m_conf->bar_get_list<string>(GetParam().first, {"def1", "def2"}), GetParam().second);
}

TEST_P(GetList, missingSectionWithDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(m_conf->get_list<string>("unknown", GetParam().first, def), def);
}

TEST_P(GetList, missingKeyWithSectionAndDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(m_conf->bar_get_list<string>(GetParam().first + "___", def), def);
}

TEST_F(BarGet, deprecated) {
  string def{"default_value"};
  EXPECT_EQ(m_conf->bar_deprecated("width", "unused", def), "100%");
  EXPECT_EQ(m_conf->bar_deprecated("unknown", "width", def), "100%");
  EXPECT_EQ(m_conf->bar_deprecated("unknown", "unknown2", def), def);
}

TEST_F(BarGet, typed) {
  EXPECT_EQ(m_conf->bar_get<string>("height"), string{"18"});
  EXPECT_EQ(m_conf->bar_get<char>("height"), '1');
  EXPECT_EQ(m_conf->bar_get<int>("height"), 18);
  EXPECT_EQ(m_conf->bar_get<short>("height"), (short)18);
  EXPECT_EQ(m_conf->bar_get<long>("height"), 18L);
  EXPECT_EQ(m_conf->bar_get<long long>("height"), 18LL);
  EXPECT_EQ(m_conf->bar_get<unsigned char>("height"), (unsigned char)18);
  EXPECT_EQ(m_conf->bar_get<unsigned short>("height"), (unsigned short)18);
  EXPECT_EQ(m_conf->bar_get<unsigned int>("height"), (unsigned int)18);
  EXPECT_EQ(m_conf->bar_get<unsigned long>("height"), (unsigned long)18);
  EXPECT_EQ(m_conf->bar_get<unsigned long long>("height"), (unsigned long long)18);
  EXPECT_EQ(m_conf->bar_get<unsigned short>("height"), (unsigned short)18);
  EXPECT_EQ(m_conf->bar_get<unsigned short>("height"), (unsigned short)18);

  EXPECT_EQ(m_conf->bar_get<bool>("bool1"), true);
  EXPECT_EQ(m_conf->bar_get<bool>("bool2"), true);
  EXPECT_EQ(m_conf->bar_get<bool>("bool3"), true);
  EXPECT_EQ(m_conf->bar_get<bool>("bool4"), true);
  EXPECT_EQ(m_conf->bar_get<bool>("bool5"), true);
  EXPECT_EQ(m_conf->bar_get<bool>("bool6"), false);

  EXPECT_FLOAT_EQ(1.234567891f, 1.23456788);
  EXPECT_FLOAT_EQ(m_conf->bar_get<float>("float"), 1.23456788);
  EXPECT_DOUBLE_EQ(m_conf->bar_get<double>("float"), 1.234567891);

  spacing_val sp_val{m_conf->bar_get<spacing_val>("spacing")};
  EXPECT_EQ(sp_val.type, spacing_type::PIXEL);
  EXPECT_EQ(sp_val.value, 100);

  extent_val ex_val{m_conf->bar_get<extent_val>("spacing")};
  EXPECT_EQ(ex_val.type, extent_type::PIXEL);
  EXPECT_EQ(ex_val.value, 100);

  percentage_with_offset p1{m_conf->bar_get<percentage_with_offset>("width")};
  EXPECT_DOUBLE_EQ(p1.percentage, 100.);
  EXPECT_EQ(p1.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p1.offset.value, 0.f);

  percentage_with_offset p2{m_conf->bar_get<percentage_with_offset>("height")};
  EXPECT_DOUBLE_EQ(p2.percentage, 0.);
  EXPECT_EQ(p2.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p2.offset.value, 18.f);

  percentage_with_offset p3{m_conf->bar_get<percentage_with_offset>("percent")};
  EXPECT_DOUBLE_EQ(p3.percentage, 36.5);
  EXPECT_EQ(p3.offset.type, extent_type::POINT);
  EXPECT_FLOAT_EQ(p3.offset.value, 42.7f);

  EXPECT_EQ(m_conf->bar_get<chrono::seconds>("height"), chrono::seconds{18});
  EXPECT_EQ(m_conf->bar_get<chrono::milliseconds>("height"), chrono::milliseconds{18});
  EXPECT_EQ(m_conf->bar_get<chrono::duration<double>>("float"), chrono::duration<double>{1.234567891});
 
  EXPECT_EQ(m_conf->bar_get<rgba>("color"), rgba{"#abc"});

  EXPECT_EQ(m_conf->bar_get<cairo_operator_t>("operator"), cairo_operator_t{CAIRO_OPERATOR_OVER});

  // Cannot test bar_get<const char *> because address sanitizer throws stack-use-after-return 
  // for any access to height_chr
  // const char *height_chr = m_conf->bar_get<const char *>("height");
  // EXPECT_EQ(strlen(height_chr), 2);
  // EXPECT_EQ(strncmp(height_chr, "18", 2), 0);
}

TEST(BadConfig, MissingBarName) {
  Config c("Ut_Bar");
  EXPECT_FALSE(c.m_conf->bar_has("modules-left"));
  EXPECT_THROW(c.m_conf->bar_get("foreground"), key_error);
  EXPECT_THROW(c.m_conf->bar_get_list("list"), key_error);
  string def{"default_value"};
  EXPECT_EQ(c.m_conf->bar_deprecated("width", "unused", def), def);
}

TEST_F(BarGet, OperatorAccess) {
  // Ok cases
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["width"].as<string>(), "100%");
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["height"].as<int>(), 18);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["fixed-center"].as<bool>(), false);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"].as<string>(), "VALUE3");
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["width"].as<string>(), "100%");
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["height"].as<int>(), 18);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["fixed-center"].as<bool>(), false);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][0]["width"].as<string>(), "100%");
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][0]["height"].as<int>(), 18);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][0]["fixed-center"].as<bool>(), false);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][1]["width"].as<string>(), "30%");
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][1]["height"].as<int>(), 8);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"][1]["fixed-center"].as<bool>(), true);
  EXPECT_EQ((*m_conf)["settings"]["compositing-border"].as<int>(8), 5);
  EXPECT_EQ((*m_conf)["modules"]["unittest_name"]["type"].as<string>(), "internal/unittest");
  EXPECT_EQ((*m_conf)["modules"]["my_script"]["type"].as<string>(), "internal/script");
  EXPECT_EQ((*m_conf)["modules"]["my_script"]["env-VAR1"].as<string>(), "VALUE1");
  EXPECT_EQ((*m_conf)["modules"]["my_script"]["env"]["VAR1"].as<string>(), "VALUE1");

  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list1"].size(), 2);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list2"].size(), 1);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"].size(), 1);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["list"].size(), 2);
  EXPECT_EQ((*m_conf)["bars"][m_conf->bar_name()]["width"].size(), 0);
  EXPECT_EQ((*m_conf)["settings"]["compositing-border"].size(), 0);
  EXPECT_EQ((*m_conf)["modules"]["my_script"].size(), 0);
  EXPECT_EQ((*m_conf)["modules"]["my_script"]["type"].size(), 0);
  EXPECT_EQ((*m_conf)["modules"]["unittest_name"]["list"].size(), 4);

  // Bad Access cases
  EXPECT_THROW((*m_conf)["barS"][m_conf->bar_name()]["width"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"]["unknown_bar"]["width"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["wiDth"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list4"][0]["width"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list4"][0]["height"].as<int>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list4"][0]["fixed-center"].as<bool>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][1]["width"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][1]["height"].as<int>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][1]["fixed-center"].as<bool>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["__width"].as<string>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["__height"].as<int>(), key_error);
  EXPECT_THROW((*m_conf)["bars"][m_conf->bar_name()]["list3"][0]["__fixed-center"].as<bool>(), key_error);
  EXPECT_THROW((*m_conf)["settings"]["missing_key"].as<int>(), key_error);
  EXPECT_THROW((*m_conf)["settings"][0].as<int>(8), runtime_error);

  EXPECT_THROW((*m_conf)["bars"].size(), key_error);
  EXPECT_THROW((*m_conf)["bars"]["unknown_bar"].size(), key_error);
  EXPECT_THROW((*m_conf)["settings"].size(), key_error);
  EXPECT_THROW((*m_conf)["modules"].size(), key_error);

  // TODO: add tests with wrong numbers of [] or wrong types (integers)
  // TODO: add tests for as() with default value
  // TODO: add tests for as_list() if accessors are not reworked maybe with also adding a size() method 
  // EXPECT_FALSE(m_conf["bars"]["modules-left"].as_list());

}