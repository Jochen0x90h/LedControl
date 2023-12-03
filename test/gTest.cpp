#include <gtest/gtest.h>
#include "../src/Color.hpp"

using namespace coco;


// Color
// -----

TEST(LedControl, Color) {
	{
		HSV hsv = {0, 0x1000, 0x1000};
		Color<int> color = toColor(hsv);
		EXPECT_EQ(color.r, 0x1000);
		EXPECT_EQ(color.g, 0);
		EXPECT_EQ(color.b, 0);
	}
	{
		HSV hsv = {64, 0x1000, 0x1000};
		Color<int> color = toColor(hsv);
		EXPECT_EQ(color.r, 0x1000);
		EXPECT_EQ(color.g, 0x400);
		EXPECT_EQ(color.b, 0);
	}
	{
		HSV hsv = {256 + 64, 0x1000, 0x1000};
		Color<int> color = toColor(hsv);
		EXPECT_EQ(color.r, 0xc00);
		EXPECT_EQ(color.g, 0x1000);
		EXPECT_EQ(color.b, 0);
	}
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	int success = RUN_ALL_TESTS();
	return success;
}
