#include <gtest/gtest.h>
#include "mute.h"

#include <vector>

TEST(Mute, InsideIntervalAllMuted) {
    Mute m(0, 1);

    std::vector<int16_t> buf = {1, 2, 3, 4, 5};
    m.process(buf.data(), buf.size());

    EXPECT_EQ(buf[0], 0);
    EXPECT_EQ(buf[1], 0);
    EXPECT_EQ(buf[2], 0);
    EXPECT_EQ(buf[3], 0);
    EXPECT_EQ(buf[4], 0);
}

TEST(Mute, BeforeIntervalNoChange) {
    Mute m(1, 2);

    std::vector<int16_t> buf = {10, 20, 30};
    m.process(buf.data(), buf.size());

    EXPECT_EQ(buf[0], 10);
    EXPECT_EQ(buf[1], 20);
    EXPECT_EQ(buf[2], 30);
}

TEST(Mute, PartialOverlapTailMuted) {
    Mute m(1, 2);

    std::vector<int16_t> dummy(44090, 123);
    m.process(dummy.data(), dummy.size());

    std::vector<int16_t> buf(20);
    for (int i = 0; i < 20; ++i) buf[i] = static_cast<int16_t>(i + 1);

    m.process(buf.data(), buf.size());

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(buf[i], i + 1);
    }
    for (int i = 10; i < 20; ++i) {
        EXPECT_EQ(buf[i], 0);
    }
}

TEST(Mute, NameIsMute) {
    Mute m(0, 1);
    EXPECT_STREQ(m.name(), "mute");
}