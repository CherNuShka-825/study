#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "FileWork.h"
#include "Universe.h"
#include "Rule.h"

static std::string readAll(const std::string& filename) {
    std::ifstream in(filename);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

TEST(FileWorkTest, LoadFromNonExistingFile) {
    Universe u_before;
    Universe u_after = u_before;

    bool ok = FileWork::loadFromFile("definitely_no_such_file_12345.life", u_after);
    EXPECT_FALSE(ok);

    EXPECT_EQ(u_after.getWidth(),  u_before.getWidth());
    EXPECT_EQ(u_after.getHeight(), u_before.getHeight());
    EXPECT_EQ(u_after.getName(),   u_before.getName());
}

TEST(FileWorkTest, LoadFromEmptyFile) {
    const std::string fname = "test_empty.life";
    {
        std::ofstream out(fname);
    }

    Universe u;
    bool ok = FileWork::loadFromFile(fname, u);
    EXPECT_FALSE(ok);
}

TEST(FileWorkTest, LoadSimpleUniverse) {
    const std::string fname = "test_simple.life";

    {
        std::ofstream out(fname);
        out << "Life 1.06\n";
        out << "#N My universe\n";
        out << "#R B3/S23\n";
        out << "1 2\n";
    }

    Universe u;
    bool ok = FileWork::loadFromFile(fname, u);
    EXPECT_TRUE(ok);

    EXPECT_EQ(u.getName(), "My universe");
    EXPECT_EQ(u.getRule().toString(), "B3/S23");

    EXPECT_EQ(u.getWidth(),  2);
    EXPECT_EQ(u.getHeight(), 3);

    for (int y = 0; y < u.getHeight(); ++y) {
        for (int x = 0; x < u.getWidth(); ++x) {
            if (x == 1 && y == 2) {
                EXPECT_TRUE(u.isAlive(x, y)) << "cell (1,2) must be alive";
            } else {
                EXPECT_FALSE(u.isAlive(x, y)) << "cell (" << x << "," << y << ") must be dead";
            }
        }
    }
}

TEST(FileWorkTest, LoadWithoutNameAndRuleUsesDefaults) {
    const std::string fname = "test_default_name_rule.life";

    {
        std::ofstream out(fname);
        out << "Life 1.06\n";
        out << "0 0\n";
    }

    Universe u;
    bool ok = FileWork::loadFromFile(fname, u);
    EXPECT_TRUE(ok);

    EXPECT_EQ(u.getName(), "Default");
    EXPECT_EQ(u.getRule().toString(), "B3/S23");
    EXPECT_EQ(u.getWidth(),  1);
    EXPECT_EQ(u.getHeight(), 1);
    EXPECT_TRUE(u.isAlive(0, 0));
}

TEST(FileWorkTest, SaveToFileBasicFormat) {
    const std::string fname = "test_save_basic.life";

    Rule r("B3/S23");
    Universe u(3, 3, r, "SaveTest");
    u.setAlive(0, 0, true);
    u.setAlive(2, 1, true);
    u.setAlive(1, 2, true);

    bool ok = FileWork::saveToFile(fname, u);
    EXPECT_TRUE(ok);

    std::string content = readAll(fname);

    EXPECT_NE(content.find("Life 1.06\n"), std::string::npos);
    EXPECT_NE(content.find("#N SaveTest\n"), std::string::npos);
    EXPECT_NE(content.find("#R B3/S23\n"), std::string::npos);

    EXPECT_NE(content.find("0 0\n"), std::string::npos);
    EXPECT_NE(content.find("2 1\n"), std::string::npos);
    EXPECT_NE(content.find("1 2\n"), std::string::npos);
}

TEST(FileWorkTest, SaveAndLoadRoundTrip) {
    const std::string fname = "test_round_trip.life";

    Rule r("B36/S245");
    Universe orig(4, 4, r, "RoundTrip");

    orig.setAlive(1, 1, true);
    orig.setAlive(2, 2, true);
    orig.setAlive(3, 0, true);

    ASSERT_TRUE(FileWork::saveToFile(fname, orig));

    Universe loaded;
    ASSERT_TRUE(FileWork::loadFromFile(fname, loaded));

    EXPECT_EQ(loaded.getName(),   orig.getName());
    EXPECT_EQ(loaded.getRule().toString(), orig.getRule().toString());

    for (int y = 0; y < orig.getHeight(); ++y) {
        for (int x = 0; x < orig.getWidth(); ++x) {
            EXPECT_EQ(loaded.isAlive(x, y), orig.isAlive(x, y))
                                << "Cell mismatch at (" << x << "," << y << ")";
        }
    }
}