#include <gtest/gtest.h>

#include "gain.h"
#include "config_parser.h"
#include "audio_stream.h"
#include "converter_factory.h"

#include <vector>
#include <cstdint>


TEST(Gain, NameAndEarlyReturn) {
    Gain g(0, 1, 2.0f);

    EXPECT_STREQ(g.name(), "gain");

    std::vector<std::int16_t> buf = {10, 20, 30};
    g.process(buf.data(), 0);
    g.process(nullptr, 3);

    EXPECT_EQ(buf[0], 10);
    EXPECT_EQ(buf[1], 20);
    EXPECT_EQ(buf[2], 30);
}


TEST(Gain, OutsideInsideAndSaturation) {
    Gain inside(0, 1, 2.0f);

    std::vector<std::int16_t> buf = {20000, -20000, 1000};
    inside.process(buf.data(), buf.size());

    EXPECT_EQ(buf[0], 32767);
    EXPECT_EQ(buf[1], -32768);
    EXPECT_EQ(buf[2], 2000);

    Gain outside(1, 2, 10.0f);
    std::vector<std::int16_t> buf2 = {7, 8, 9};
    outside.process(buf2.data(), buf2.size());

    EXPECT_EQ(buf2[0], 7);
    EXPECT_EQ(buf2[1], 8);
    EXPECT_EQ(buf2[2], 9);
}

TEST(Gain, FactoryCanCreateRegisteredGain) {
    ConverterFactory* f = ConverterFactory::getInstance();

    std::vector<WavReader*> inputs;
    f->set_inputs(inputs);

    ConfigCommand cmd;
    cmd.name = "gain";
    cmd.args = {"0", "1", "1.0"};

    auto conv = f->create(cmd);
    ASSERT_NE(conv, nullptr);
    EXPECT_STREQ(conv->name(), "gain");
}
