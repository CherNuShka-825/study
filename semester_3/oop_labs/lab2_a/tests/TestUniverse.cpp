#include <gtest/gtest.h>
#include "Universe.h"


static void expectAllDead(const Universe& u) {
    int w = u.getWidth();
    int h = u.getHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            EXPECT_FALSE(u.isAlive(x, y)) << "cell (" << x << "," << y << ") must be dead";
        }
    }
}

TEST(UniverseTest, ParamConstructorBasicPropertimes) {
    Rule r;
    Universe u(5, 7, r, "Test");

    EXPECT_EQ(u.getWidth(), 5);
    EXPECT_EQ(u.getHeight(), 7);
    EXPECT_EQ(u.getName(), "Test");
    EXPECT_EQ(u.getTick(), 0);

    expectAllDead(u);
}

TEST(UniverseTest, DefaultConstructorSeedsPattern) {
    Universe u;

    EXPECT_EQ(u.getWidth(), 20);
    EXPECT_EQ(u.getHeight(), 20);
    EXPECT_EQ(u.getName(), "Default");
    EXPECT_EQ(u.getTick(), 0);

    EXPECT_TRUE(u.isAlive(4, 4));
    EXPECT_TRUE(u.isAlive(5, 5));
    EXPECT_TRUE(u.isAlive(6, 3));
    EXPECT_TRUE(u.isAlive(6, 4));
    EXPECT_TRUE(u.isAlive(6, 5));
}

TEST(UniverseTest, SetAndGetAliveValid) {
    Rule r;
    Universe u(3, 3, r, "U");

    expectAllDead(u);

    u.setAlive(1, 1, true);
    EXPECT_TRUE(u.isAlive(1, 1));

    u.setAlive(1, 1, false);
    EXPECT_FALSE(u.isAlive(1, 1));
}

TEST(UniverseTest, SetAliveInvalidDoesNotModifyField) {
    Rule r;
    Universe u(3, 3, r, "U");

    u.setAlive(0, 0, true);
    EXPECT_TRUE(u.isAlive(0, 0));

    u.setAlive(-1, 0, true);
    u.setAlive(0, -1, true);
    u.setAlive(3, 0, true);
    u.setAlive(0, 3, true);

    EXPECT_TRUE(u.isAlive(0, 0));
}

TEST(UniverseTest, BlinkerOscillator) {
    Rule r;
    Universe u(5, 5, r, "Blinker");

    u.setAlive(2, 1, true);
    u.setAlive(2, 2, true);
    u.setAlive(2, 3, true);

    u.tick();
    EXPECT_TRUE(u.isAlive(1, 2));
    EXPECT_TRUE(u.isAlive(2, 2));
    EXPECT_TRUE(u.isAlive(3, 2));

    EXPECT_FALSE(u.isAlive(2, 1));
    EXPECT_FALSE(u.isAlive(2, 3));

    u.tick();
    EXPECT_TRUE(u.isAlive(2, 1));
    EXPECT_TRUE(u.isAlive(2, 2));
    EXPECT_TRUE(u.isAlive(2, 3));

    EXPECT_FALSE(u.isAlive(1, 2));
    EXPECT_FALSE(u.isAlive(3, 2));
}

TEST(UniverseTest, TickNAndNegative) {
    Rule r;
    Universe u(5, 5, r, "Ticks");

    EXPECT_EQ(u.getTick(), 0);

    u.tick(3);
    EXPECT_EQ(u.getTick(), 3);

    u.tick(-5);
    EXPECT_EQ(u.getTick(), 3);
}

TEST(UniverseTest, ToStringSmallField) {
    Rule r;
    Universe u(2, 2, r, "Small");

    u.setAlive(0, 0, true);
    u.setAlive(1, 1, true);

    std::string expected = "⬜⬛\n⬛⬜";
    EXPECT_EQ(u.toString(), expected);
}
