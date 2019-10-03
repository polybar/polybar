#include <tuple>

#include "common/test.hpp"
#include "components/builder.hpp"
#include "components/types.hpp"
#include "utils/factory.hpp"
#include "drawtypes/label.hpp"

using namespace polybar;
using namespace std;

/**
 * \brief Testing-only subclass of builder to change access level
 */
class TestableBuilder : public builder {
  using builder::builder;
  public: using builder::get_label_text;
};

class Builder : public ::testing::Test {
  protected:

    /**
     * Generic bar settings
     *
     * Builder only needs spacing and background
     */
    bar_settings m_bar{};
    TestableBuilder m_builder{m_bar};
};

// GetLabelTextTest {{{

/**
 * \brief Class for parameterized tests on get_label_text
 *
 * The first element of the pair is the expected returned text, the second
 * element is a triple containing the original label text, m_ellipsis and
 * m_maxlen, in that order
 */
class GetLabelTextTest :
  public Builder,
  public ::testing::WithParamInterface<pair<string, tuple<string, bool, int>>> {};

vector<pair<string, tuple<string, bool, int>>> get_label_text_list = {
  {"...", make_tuple("abcd", true, 3)},
  {"abc", make_tuple("abc", true, 3)},
  {"abc", make_tuple("abcdefgh", false, 3)},
  {"a...", make_tuple("abcdefgh", true, 4)},
  {"abcd...", make_tuple("abcdefgh", true, 7)},
  {"abcdefgh", make_tuple("abcdefgh", true, 8)},
};

INSTANTIATE_TEST_SUITE_P(Inst, GetLabelTextTest,
    ::testing::ValuesIn(get_label_text_list));

TEST_P(GetLabelTextTest, correctness) {
  label_t m_label = factory_util::shared<label>(get<0>(GetParam().second));
  m_label->m_ellipsis = get<1>(GetParam().second);
  m_label->m_maxlen = get<2>(GetParam().second);

  auto text = m_builder.get_label_text(m_label);
  EXPECT_EQ(GetParam().first, text);

  EXPECT_LE(text.length(), m_label->m_maxlen) << "Returned text is longer than maxlen";
}

// }}}
