#include <gtest/gtest.h>

#include "pipeline.h"
#include "audio_stream.h"
#include "converter.h"

#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdint>

static void rm(const std::string& p) { std::remove(p.c_str()); }

static void makewav(const std::string& p, const std::vector<std::int16_t>& s) {
    rm(p);
    WavInfo info{};
    info.sampleRate    = 44100;
    info.bitsPerSample = 16;
    info.numChannels   = 1;
    info.dataSizeBytes = static_cast<std::uint32_t>(s.size() * 2);
    info.totalSamples  = static_cast<std::uint32_t>(s.size());
    WavWriter w(p, info);
    w.writeSamples(s.data(), s.size());
}

TEST(Pipeline, CopyWithoutCommands) {
    std::string inPath  = "pipe_in.wav";
    std::string outPath = "pipe_out.wav";
    std::string cfgPath = "pipe_empty.cfg";

    std::vector<std::int16_t> src = {10, -20, 30, -40, 50};
    makewav(inPath, src);

    {
        std::ofstream cfg(cfgPath);
    }

    std::vector<std::string> inputs = {inPath};
    runPipeline(cfgPath, outPath, inputs);

    WavReader r(outPath);
    std::vector<std::int16_t> dst(src.size());
    std::size_t n = r.readSamples(dst.data(), dst.size());

    EXPECT_EQ(n, src.size());
    EXPECT_EQ(dst, src);

    rm(inPath);
    rm(outPath);
    rm(cfgPath);
}

TEST(Pipeline, MuteWholeFile) {
    std::string inPath  = "pipe_mute_in.wav";
    std::string outPath = "pipe_mute_out.wav";
    std::string cfgPath = "pipe_mute.cfg";

    std::vector<std::int16_t> src(100, 100);
    makewav(inPath, src);

    {
        std::ofstream cfg(cfgPath);
        cfg << "mute 0 1\n";
    }

    std::vector<std::string> inputs = {inPath};
    runPipeline(cfgPath, outPath, inputs);

    WavReader r(outPath);
    std::vector<std::int16_t> dst(src.size());
    std::size_t n = r.readSamples(dst.data(), dst.size());

    EXPECT_EQ(n, src.size());
    for (auto v : dst) {
        EXPECT_EQ(v, 0);
    }

    rm(inPath);
    rm(outPath);
    rm(cfgPath);
}

TEST(Pipeline, MixTwoInputs) {
    std::string in1Path = "pipe_mix_in1.wav";
    std::string in2Path = "pipe_mix_in2.wav";
    std::string outPath = "pipe_mix_out.wav";
    std::string cfgPath = "pipe_mix.cfg";

    std::vector<std::int16_t> src1 = {100, 100, 100, 100};
    std::vector<std::int16_t> src2 = {0, 200, -200, 100};
    makewav(in1Path, src1);
    makewav(in2Path, src2);

    {
        std::ofstream cfg(cfgPath);
        cfg << "mix $2\n";
    }

    std::vector<std::string> inputs = {in1Path, in2Path};
    runPipeline(cfgPath, outPath, inputs);

    WavReader r(outPath);
    std::vector<std::int16_t> dst(src1.size());
    std::size_t n = r.readSamples(dst.data(), dst.size());

    EXPECT_EQ(n, src1.size());
    EXPECT_EQ(dst[0], static_cast<std::int16_t>((100 +   0) / 2));
    EXPECT_EQ(dst[1], static_cast<std::int16_t>((100 + 200) / 2));
    EXPECT_EQ(dst[2], static_cast<std::int16_t>((100 - 200) / 2));
    EXPECT_EQ(dst[3], static_cast<std::int16_t>((100 + 100) / 2));

    rm(in1Path);
    rm(in2Path);
    rm(outPath);
    rm(cfgPath);
}

TEST(Pipeline, UnknownConverterThrows) {
    std::string inPath  = "pipe_unknown_in.wav";
    std::string outPath = "pipe_unknown_out.wav";
    std::string cfgPath = "pipe_unknown.cfg";

    std::vector<std::int16_t> src = {1, 2, 3, 4};
    makewav(inPath, src);

    {
        std::ofstream cfg(cfgPath);
        cfg << "abracadabra 1 2 3\n";
    }

    std::vector<std::string> inputs = {inPath};

    EXPECT_THROW(
            runPipeline(cfgPath, outPath, inputs),
            ConverterError
    );

    rm(inPath);
    rm(outPath);
    rm(cfgPath);
}