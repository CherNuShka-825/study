#include <gtest/gtest.h>

#include "config_parser.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static void removeFileIfExists(const std::string& path) {
    std::remove(path.c_str());
}

// ------------------------------------------------------------
// 1. Файл не найден
// ------------------------------------------------------------
TEST(ConfigParser, ThrowsIfFileNotFound) {
    std::string fileName = "config_does_not_exist_12345.txt";
    removeFileIfExists(fileName);

    EXPECT_THROW(
            {
                ConfigParser parser(fileName);
                auto cmds = parser.parse();
                (void)cmds;
            },
            ConfigError
    );
}

// ------------------------------------------------------------
// 2. Пустой файл → пустой список команд
// ------------------------------------------------------------
TEST(ConfigParser, EmptyFileGivesNoCommands) {
    const std::string fileName = "config_empty.txt";
    removeFileIfExists(fileName);

    {
        std::ofstream out(fileName);
        // ничего не пишем
    }

    ConfigParser parser(fileName);
    auto cmds = parser.parse();

    EXPECT_TRUE(cmds.empty());

    removeFileIfExists(fileName);
}

// ------------------------------------------------------------
// 3. Пропуск пустых строк и комментариев
// ------------------------------------------------------------
TEST(ConfigParser, SkipsEmptyLinesAndComments) {
    const std::string fileName = "config_comments.txt";
    removeFileIfExists(fileName);

    {
        std::ofstream out(fileName);
        out << "\n";                        // 1: пустая
        out << "   \t  \n";                 // 2: пробельная
        out << "# full line comment\n";     // 3: комментарий
        out << "   # spaced comment\n";     // 4: комментарий с пробелами
        out << "mute 0 10\n";               // 5: первая команда
        out << "   gain  1  3  0.5   \n";   // 6: вторая команда с пробелами
    }

    ConfigParser parser(fileName);
    auto cmds = parser.parse();

    ASSERT_EQ(cmds.size(), 2u);

    // Первая команда
    EXPECT_EQ(cmds[0].line, 5u);
    EXPECT_EQ(cmds[0].name, "mute");
    ASSERT_EQ(cmds[0].args.size(), 2u);
    EXPECT_EQ(cmds[0].args[0], "0");
    EXPECT_EQ(cmds[0].args[1], "10");

    // Вторая команда
    EXPECT_EQ(cmds[1].line, 6u);
    EXPECT_EQ(cmds[1].name, "gain");
    ASSERT_EQ(cmds[1].args.size(), 3u);
    EXPECT_EQ(cmds[1].args[0], "1");
    EXPECT_EQ(cmds[1].args[1], "3");
    EXPECT_EQ(cmds[1].args[2], "0.5");

    removeFileIfExists(fileName);
}

// ------------------------------------------------------------
// 4. Разделение по пробелам: несколько пр
