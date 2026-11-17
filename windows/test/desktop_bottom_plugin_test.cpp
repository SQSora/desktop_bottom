#include <gtest/gtest.h>

namespace desktop_bottom {
namespace test {

TEST(DesktopBottomPlugin, Placeholder) {
  // Native behavior depends on a real top-level window and is verified
  // manually; Dart-side behavior is covered by desktop_bottom_test.
  EXPECT_TRUE(true);
}

}  // namespace test
}  // namespace desktop_bottom
