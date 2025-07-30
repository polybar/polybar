#include "components/config.hpp"
#include "cairo/utils.hpp"

#include "common/test.hpp"

using namespace polybar;
using namespace std;


/**
 * \brief Fixture class
 */
class Config : public ::testing::Test {
 public:
  Config(): ::testing::Test() {
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
          {"env-VAR1", "VALUE1"},
          {"env-VAR2", "VALUE2"},
          {"env-VAR3", "VALUE3"},
          {"env2-VAR1", "VALUE1"},
          {"list1-0", "VALUE0"},
          {"list1-1", "VALUE1"},
          {"list2-0", "VALUE0"},
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
          {"type", "internal/unittest"}
        }
      }
    });
  }
 protected:
  const logger l = logger(loglevel::NONE);
  unique_ptr<config> m_conf = make_unique<config>(l, "/dev/zero", "ut_bar");
};

class HasTest : public Config, public ::testing::WithParamInterface<pair<pair<string, string>, bool>> {};

vector<pair<pair<string, string>, bool>> has_test_input = {
  {{"settings", "compositing-border"}, true},
  {{"bar/ut_bar", "modules-left"}, true},
  {{"modules/unittest_name", "type"}, true},
  {{"settingS", "compositing-border"}, false},
  {{"bar/UT_bar", "modules-left"}, false},
  {{"modules/unittest_name", "TYPE"}, false}
};

INSTANTIATE_TEST_SUITE_P(Inst, HasTest, ::testing::ValuesIn(has_test_input));

TEST_P(HasTest, correctness) {
  EXPECT_EQ(m_conf->has(GetParam().first.first, GetParam().first.second), GetParam().second);
}

class Get : public Config, public ::testing::WithParamInterface<pair<string, string>> {};

vector<pair<string, string>> get_input = {
  {"modules-left", "unittest_name"},
  {"width", "100%"},
  {"height", "18"},
  {"fixed-center", "false"}
};

INSTANTIATE_TEST_SUITE_P(Inst, Get, ::testing::ValuesIn(get_input));

TEST_P(Get, found) {
  EXPECT_EQ(m_conf->get(GetParam().first), GetParam().second);
}

TEST_P(Get, missing) {
  EXPECT_THROW(m_conf->get(GetParam().first + "___"), key_error);
}

class GetWithSection : public Config, public ::testing::WithParamInterface<pair<pair<string, string>, string>> {};

vector<pair<pair<string, string>, string>> get_with_section_input = {
  {{"settings", "compositing-border"}, "5"},
  {{"bar/ut_bar", "modules-left"}, "unittest_name"},
  {{"bar/ut_bar", "width"}, "100%"},
  {{"bar/ut_bar", "height"}, "18"},
  {{"bar/ut_bar", "fixed-center"}, "false"},
  {{"modules/unittest_name", "type"}, "internal/unittest"},
};

INSTANTIATE_TEST_SUITE_P(Inst, GetWithSection, ::testing::ValuesIn(get_with_section_input));

TEST_P(GetWithSection, found) {
  EXPECT_EQ(m_conf->get(GetParam().first.first, GetParam().first.second), GetParam().second);
}

TEST_P(GetWithSection, missingSection) {
  EXPECT_THROW(m_conf->get(GetParam().first.first + "___", GetParam().first.second), key_error);
}

TEST_P(GetWithSection, missingKey) {
  EXPECT_THROW(m_conf->get(GetParam().first.first, GetParam().first.second + "___"), key_error);
}

TEST_P(GetWithSection, foundWithDefaultValue) {
  EXPECT_EQ(m_conf->get(GetParam().first.first, GetParam().first.second, string("default_value")), GetParam().second);
}

TEST_P(GetWithSection, missingSectionWithDefaultValue) {
  EXPECT_EQ(m_conf->get(GetParam().first.first + "___", GetParam().first.second, string("default_value")), "default_value");
}

TEST_P(GetWithSection, missingKeyWithDefaultValue) {
  EXPECT_EQ(m_conf->get(GetParam().first.first, GetParam().first.second + "___", string("default_value")), "default_value");
}

class GetWithPrefix : public Config, public ::testing::WithParamInterface<pair<pair<string, string>, vector<pair<string, string>>>> {};

vector<pair<pair<string, string>, vector<pair<string, string>>>> get_with_prefix_input = {
  {{"bar/ut_bar", "env-"}, {{"VAR1", "VALUE1"}, {"VAR2", "VALUE2"}, {"VAR3", "VALUE3"}}},
  {{"bar/ut_bar", "env2-"}, {{"VAR1", "VALUE1"}}}
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

class GetList : public Config, public ::testing::WithParamInterface<pair<string, vector<string>>> {};

vector<pair<string, vector<string>>> get_list_input = {
  {"list1", {"VALUE0", "VALUE1"}},
  {"list2", {"VALUE0"}}
};

INSTANTIATE_TEST_SUITE_P(Inst, GetList, ::testing::ValuesIn(get_list_input));

TEST_P(GetList, found) {
  EXPECT_EQ(m_conf->get_list(GetParam().first), GetParam().second);
}

TEST_P(GetList, foundWithSection) {
  EXPECT_EQ(m_conf->get_list("bar/ut_bar", GetParam().first), GetParam().second);
}

TEST_P(GetList, missingSection) {
  EXPECT_THROW(m_conf->get_list("unknown", GetParam().first), key_error);
}

TEST_P(GetList, missingKey) {
  EXPECT_THROW(m_conf->get_list(GetParam().first + "___"), key_error);
}

TEST_P(GetList, missingKeyWithSection) {
  EXPECT_THROW(m_conf->get_list("bar/ut_bar", GetParam().first + "___"), key_error);
}

TEST_P(GetList, foundWithSectionAndDefault) {
  EXPECT_EQ(m_conf->get_list<string>("bar/ut_bar", GetParam().first, {"def1", "def2"}), GetParam().second);
}

TEST_P(GetList, missingSectionWithDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(m_conf->get_list<string>("unknown", GetParam().first, def), def);
}

TEST_P(GetList, missingKeyWithSectionAndDefault) {
  vector<string> def{"def1", "def2"};
  EXPECT_EQ(m_conf->get_list<string>("bar/ut_bar", GetParam().first + "___", def), def);
}

TEST_F(Config, deprecated) {
  string def{"default_value"};
  EXPECT_EQ(m_conf->deprecated("bar/ut_bar", "width", "unused", def), "100%");
  EXPECT_EQ(m_conf->deprecated("bar/ut_bar", "unknown", "width", def), "100%");
  EXPECT_EQ(m_conf->deprecated("unknown", "width", "unused", def), def);
  EXPECT_EQ(m_conf->deprecated("bar/ut_bar", "unknown", "unknown2", def), def);
}

TEST_F(Config, typed) {
  EXPECT_EQ(m_conf->get<string>("bar/ut_bar", "height"), string{"18"});
  EXPECT_EQ(m_conf->get<char>("bar/ut_bar", "height"), '1');
  EXPECT_EQ(m_conf->get<int>("bar/ut_bar", "height"), 18);
  EXPECT_EQ(m_conf->get<short>("bar/ut_bar", "height"), (short)18);
  EXPECT_EQ(m_conf->get<long>("bar/ut_bar", "height"), 18L);
  EXPECT_EQ(m_conf->get<long long>("bar/ut_bar", "height"), 18LL);
  EXPECT_EQ(m_conf->get<unsigned char>("bar/ut_bar", "height"), (unsigned char)18);
  EXPECT_EQ(m_conf->get<unsigned short>("bar/ut_bar", "height"), (unsigned short)18);
  EXPECT_EQ(m_conf->get<unsigned int>("bar/ut_bar", "height"), (unsigned int)18);
  EXPECT_EQ(m_conf->get<unsigned long>("bar/ut_bar", "height"), (unsigned long)18);
  EXPECT_EQ(m_conf->get<unsigned long long>("bar/ut_bar", "height"), (unsigned long long)18);
  EXPECT_EQ(m_conf->get<unsigned short>("bar/ut_bar", "height"), (unsigned short)18);
  EXPECT_EQ(m_conf->get<unsigned short>("bar/ut_bar", "height"), (unsigned short)18);

  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool1"), true);
  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool2"), true);
  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool3"), true);
  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool4"), true);
  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool5"), true);
  EXPECT_EQ(m_conf->get<bool>("bar/ut_bar", "bool6"), false);

  EXPECT_FLOAT_EQ(1.234567891f, 1.23456788);
  EXPECT_FLOAT_EQ(m_conf->get<float>("bar/ut_bar", "float"), 1.23456788);
  EXPECT_DOUBLE_EQ(m_conf->get<double>("bar/ut_bar", "float"), 1.234567891);

  spacing_val sp_val{m_conf->get<spacing_val>("bar/ut_bar", "spacing")};
  EXPECT_EQ(sp_val.type, spacing_type::PIXEL);
  EXPECT_EQ(sp_val.value, 100);

  extent_val ex_val{m_conf->get<extent_val>("bar/ut_bar", "spacing")};
  EXPECT_EQ(ex_val.type, extent_type::PIXEL);
  EXPECT_EQ(ex_val.value, 100);

  percentage_with_offset p1{m_conf->get<percentage_with_offset>("bar/ut_bar", "width")};
  EXPECT_DOUBLE_EQ(p1.percentage, 100.);
  EXPECT_EQ(p1.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p1.offset.value, 0.f);

  percentage_with_offset p2{m_conf->get<percentage_with_offset>("bar/ut_bar", "height")};
  EXPECT_DOUBLE_EQ(p2.percentage, 0.);
  EXPECT_EQ(p2.offset.type, extent_type::PIXEL);
  EXPECT_FLOAT_EQ(p2.offset.value, 18.f);

  percentage_with_offset p3{m_conf->get<percentage_with_offset>("bar/ut_bar", "percent")};
  EXPECT_DOUBLE_EQ(p3.percentage, 36.5);
  EXPECT_EQ(p3.offset.type, extent_type::POINT);
  EXPECT_FLOAT_EQ(p3.offset.value, 42.7f);

  EXPECT_EQ(m_conf->get<chrono::seconds>("bar/ut_bar", "height"), chrono::seconds{18});
  EXPECT_EQ(m_conf->get<chrono::milliseconds>("bar/ut_bar", "height"), chrono::milliseconds{18});
  EXPECT_EQ(m_conf->get<chrono::duration<double>>("bar/ut_bar", "float"), chrono::duration<double>{1.234567891});
 
  EXPECT_EQ(m_conf->get<rgba>("bar/ut_bar", "color"), rgba{"#abc"});

  EXPECT_EQ(m_conf->get<cairo_operator_t>("bar/ut_bar", "operator"), cairo_operator_t{CAIRO_OPERATOR_OVER});

  // Cannot test get<const char *> because address sanitizer throws stack-use-after-return 
  // for any access to height_chr
  // const char *height_chr = m_conf->get<const char *>("bar/ut_bar", "height");
  // EXPECT_EQ(strlen(height_chr), 2);
  // EXPECT_EQ(strncmp(height_chr, "18", 2), 0);

}

