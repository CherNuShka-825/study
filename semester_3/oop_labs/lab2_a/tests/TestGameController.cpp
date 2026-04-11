#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>
#include "GameController.h"

// --- перехват stdout ---
struct CoutCapture {
    std::streambuf* oldBuf;
    std::ostringstream oss;

    CoutCapture()
            : oldBuf(std::cout.rdbuf(oss.rdbuf()))
    {}

    ~CoutCapture() {
        std::cout.rdbuf(oldBuf);
    }

    std::string str() const { return oss.str(); }
};

// --- перехват stderr ---
struct CerrCapture {
    std::streambuf* oldBuf;
    std::ostringstream oss;

    CerrCapture()
            : oldBuf(std::cerr.rdbuf(oss.rdbuf()))
    {}

    ~CerrCapture() {
        std::cerr.rdbuf(oldBuf);
    }

    std::string str() const { return oss.str(); }
};

TEST(GameControllerRunTest, HelpThenExit) {
    std::istringstream input("help\nexit\n");
    auto* oldIn = std::cin.rdbuf(input.rdbuf());

    CoutCapture outCap;

    GameController gc;
    gc.run();

    std::cin.rdbuf(oldIn);

    std::string out = outCap.str();

    EXPECT_NE(out.find("Available commands:"), std::string::npos);
    EXPECT_NE(out.find("tick [n] / t [n]"), std::string::npos);
    EXPECT_NE(out.find("dump <filename>"), std::string::npos);
    EXPECT_NE(out.find("exit"), std::string::npos);

    EXPECT_NE(out.find("Exiting."), std::string::npos);
}

TEST(GameControllerRunTest, TickThreeThenExit) {
    std::istringstream input("tick 3\nexit\n");
    auto* oldIn = std::cin.rdbuf(input.rdbuf());

    CoutCapture outCap;

    GameController gc;
    gc.run();

    std::cin.rdbuf(oldIn);

    std::string out = outCap.str();

    EXPECT_NE(out.find("Tick: 3"), std::string::npos);
}

TEST(GameControllerRunTest, TickZeroShowsErrorNoTick) {
    std::istringstream input("tick 0\nexit\n");
    auto* oldIn = std::cin.rdbuf(input.rdbuf());

    CoutCapture outCap;
    CerrCapture errCap;

    GameController gc;
    gc.run();

    std::cin.rdbuf(oldIn);

    std::string out = outCap.str();
    std::string err = errCap.str();

    EXPECT_NE(err.find("Tick count must be positive"), std::string::npos);
    EXPECT_EQ(out.find("Tick:"), std::string::npos);
}

TEST(GameControllerRunTest, UnknownCommandThenExit) {
    std::istringstream input("foobar\nexit\n");
    auto* oldIn = std::cin.rdbuf(input.rdbuf());

    CoutCapture outCap;
    CerrCapture errCap;

    GameController gc;
    gc.run();

    std::cin.rdbuf(oldIn);

    std::string err = errCap.str();

    EXPECT_NE(err.find("Unknown command"), std::string::npos);
    EXPECT_NE(err.find("Type 'help' for available commands."), std::string::npos);
}

TEST(GameControllerRunTest, DumpCreatesFileThroughRun) {
    const std::string fname = "test_gc_run_dump.life";
    std::remove(fname.c_str());

    std::istringstream input(std::string("dump ") + fname + "\nexit\n");
    auto* oldIn = std::cin.rdbuf(input.rdbuf());

    CoutCapture outCap;
    CerrCapture errCap;

    GameController gc;
    gc.run();

    std::cin.rdbuf(oldIn);

    std::string err = errCap.str();
    EXPECT_EQ(err.find("Failed to save universe"), std::string::npos);

    std::ifstream in(fname);
    ASSERT_TRUE(in.good()) << "Dump file was not created via run()";

    std::string firstLine;
    std::getline(in, firstLine);
    EXPECT_EQ(firstLine, "Life 1.06");
}

TEST(GameControllerCtorTest, LoadsUniverseFromFile) {
    const std::string fname = "gc_ctor_ok.life";

    {
        std::ofstream out(fname);
        out << "Life 1.06\n";
        out << "#N FromFile\n";
        out << "#R B3/S23\n";
        out << "1 1\n";
    }

    CoutCapture outCap;
    CerrCapture errCap;

    GameController gc(fname);

    std::string out = outCap.str();
    std::string err = errCap.str();

    EXPECT_NE(out.find("Loaded universe from file: " + fname), std::string::npos);
    EXPECT_EQ(err.find("Failed to load universe from file"), std::string::npos);
}
