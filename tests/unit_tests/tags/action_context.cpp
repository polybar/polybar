#include "tags/context.hpp"

#include "common/test.hpp"
#include "components/logger.hpp"

using namespace polybar;
using namespace std;
using namespace tags;

TEST(ActionCtxtTest, dblClick) {
  action_context ctxt;

  ctxt.action_open(mousebtn::DOUBLE_LEFT, "", alignment::LEFT);
  ctxt.action_close(mousebtn::DOUBLE_LEFT, alignment::LEFT);

  ASSERT_TRUE(ctxt.has_double_click());

  ctxt.reset();

  ctxt.action_open(mousebtn::DOUBLE_MIDDLE, "", alignment::LEFT);
  ctxt.action_close(mousebtn::DOUBLE_MIDDLE, alignment::LEFT);

  ASSERT_TRUE(ctxt.has_double_click());

  ctxt.action_open(mousebtn::DOUBLE_RIGHT, "", alignment::LEFT);
  ctxt.action_close(mousebtn::DOUBLE_RIGHT, alignment::LEFT);

  ASSERT_TRUE(ctxt.has_double_click());
}

TEST(ActionCtxtTest, closing) {
  action_context ctxt;

  auto id1 = ctxt.action_open(mousebtn::LEFT, "", alignment::LEFT);
  auto id2 = ctxt.action_open(mousebtn::RIGHT, "", alignment::CENTER);
  auto id3 = ctxt.action_open(mousebtn::RIGHT, "", alignment::LEFT);
  auto id4 = ctxt.action_open(mousebtn::MIDDLE, "", alignment::LEFT);

  EXPECT_NE(NO_ACTION, id1);
  EXPECT_NE(NO_ACTION, id2);
  EXPECT_NE(NO_ACTION, id3);
  EXPECT_NE(NO_ACTION, id4);

  EXPECT_EQ(make_pair(id1, mousebtn::LEFT), ctxt.action_close(mousebtn::LEFT, alignment::LEFT));
  EXPECT_EQ(make_pair(id4, mousebtn::MIDDLE), ctxt.action_close(mousebtn::NONE, alignment::LEFT));
  EXPECT_EQ(make_pair(id3, mousebtn::RIGHT), ctxt.action_close(mousebtn::NONE, alignment::LEFT));
  EXPECT_EQ(make_pair(id2, mousebtn::RIGHT), ctxt.action_close(mousebtn::NONE, alignment::CENTER));

  EXPECT_EQ(4, ctxt.num_actions());
}

TEST(ActionCtxtTest, overlapping) {
  action_context ctxt;

  /*
   * clang-format off
   *
   * Sets up the following overlapping of actions:
   *    0123456
   * 1: [--)     (LEFT)
   * 2:  [----)  (MIDDLE)
   * 3:   [--)   (RIGHT)
   * clang-format on
   */

  auto id1 = ctxt.action_open(mousebtn::LEFT, "", alignment::LEFT);
  auto id2 = ctxt.action_open(mousebtn::MIDDLE, "", alignment::LEFT);
  auto id3 = ctxt.action_open(mousebtn::RIGHT, "", alignment::LEFT);
  EXPECT_EQ(make_pair(id1, mousebtn::LEFT), ctxt.action_close(mousebtn::LEFT, alignment::LEFT));
  EXPECT_EQ(make_pair(id3, mousebtn::RIGHT), ctxt.action_close(mousebtn::RIGHT, alignment::LEFT));
  EXPECT_EQ(make_pair(id2, mousebtn::MIDDLE), ctxt.action_close(mousebtn::MIDDLE, alignment::LEFT));

  ctxt.set_start(id1, 0);
  ctxt.set_end(id1, 3);
  ctxt.set_start(id2, 1);
  ctxt.set_end(id2, 6);
  ctxt.set_start(id3, 2);
  ctxt.set_end(id3, 5);

  auto actions = ctxt.get_actions(2);

  EXPECT_EQ(id1, actions[mousebtn::LEFT]);
  EXPECT_EQ(id2, actions[mousebtn::MIDDLE]);
  EXPECT_EQ(id3, actions[mousebtn::RIGHT]);

  EXPECT_EQ(3, ctxt.num_actions());
}

TEST(ActionCtxtTest, stacking) {
  action_context ctxt;

  /*
   * clang-format off
   *
   * Sets up the following stacked actions:
   *    012345678
   * 1: [-------)
   * 2:  [-----)
   * 3:    [--)
   * clang-format on
   */

  auto id1 = ctxt.action_open(mousebtn::LEFT, "", alignment::LEFT);
  auto id2 = ctxt.action_open(mousebtn::LEFT, "", alignment::LEFT);
  auto id3 = ctxt.action_open(mousebtn::LEFT, "", alignment::LEFT);
  EXPECT_EQ(make_pair(id3, mousebtn::LEFT), ctxt.action_close(mousebtn::NONE, alignment::LEFT));
  EXPECT_EQ(make_pair(id2, mousebtn::LEFT), ctxt.action_close(mousebtn::NONE, alignment::LEFT));
  EXPECT_EQ(make_pair(id1, mousebtn::LEFT), ctxt.action_close(mousebtn::NONE, alignment::LEFT));

  ctxt.set_start(id1, 0);
  ctxt.set_end(id1, 8);
  ctxt.set_start(id2, 1);
  ctxt.set_end(id2, 7);
  ctxt.set_start(id3, 3);
  ctxt.set_end(id3, 6);

  EXPECT_EQ(id1, ctxt.has_action(mousebtn::LEFT, 0));
  EXPECT_EQ(id2, ctxt.has_action(mousebtn::LEFT, 1));
  EXPECT_EQ(id2, ctxt.has_action(mousebtn::LEFT, 2));
  EXPECT_EQ(id3, ctxt.has_action(mousebtn::LEFT, 3));
  EXPECT_EQ(id3, ctxt.has_action(mousebtn::LEFT, 4));
  EXPECT_EQ(id3, ctxt.has_action(mousebtn::LEFT, 5));
  EXPECT_EQ(id2, ctxt.has_action(mousebtn::LEFT, 6));
  EXPECT_EQ(id1, ctxt.has_action(mousebtn::LEFT, 7));

  EXPECT_EQ(3, ctxt.num_actions());
}

TEST(ActionCtxtTest, cmd) {
  action_context ctxt;

  string cmd = "foobar";

  auto id = ctxt.action_open(mousebtn::DOUBLE_RIGHT, cmd.substr(), alignment::RIGHT);

  ASSERT_EQ(cmd, ctxt.get_action(id));
}
