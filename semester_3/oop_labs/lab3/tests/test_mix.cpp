#include <gtest/gtest.h>
#include "mix.h"
#include "audio_stream.h"

#include <vector>
#include <cstdio>

static void rm(const std::string& p){ std::remove(p.c_str()); }

static void makewav(const std::string& p, const std::vector<int16_t>& s){
    rm(p);
    WavInfo info;
    info.sampleRate=44100;
    info.bitsPerSample=16;
    info.numChannels=1;
    info.dataSizeBytes=s.size()*2;
    info.totalSamples=s.size();
    WavWriter w(p, info);
    w.writeSamples(s.data(), s.size());
}

TEST(Mix, Average) {
    std::string p="mix_avg.wav";
    makewav(p, {10,20,30,40});
    WavReader r(p);
    Mix m(r,0);

    std::vector<int16_t> buf = {100,100,100,100};
    m.process(buf.data(), buf.size());

    EXPECT_EQ(buf[0], (100+10)/2);
    EXPECT_EQ(buf[1], (100+20)/2);
    EXPECT_EQ(buf[2], (100+30)/2);
    EXPECT_EQ(buf[3], (100+40)/2);

    rm(p);
}

TEST(Mix, ExtraShorter) {
    std::string p="mix_short.wav";
    makewav(p, {10,20});
    WavReader r(p);
    Mix m(r,0);

    std::vector<int16_t> buf = {100,100,100,100};
    m.process(buf.data(), buf.size());

    EXPECT_EQ(buf[0], (100+10)/2);
    EXPECT_EQ(buf[1], (100+20)/2);
    EXPECT_EQ(buf[2], 100);
    EXPECT_EQ(buf[3], 100);

    rm(p);
}

TEST(Mix, InsertLater) {
    std::string p="mix_ins.wav";
    makewav(p, {0,100});
    WavReader r(p);
    Mix m(r,1);

    const size_t n1=44100, n2=2;
    std::vector<int16_t> buf(n1+n2, 1000);

    m.process(buf.data(), n1);
    m.process(buf.data()+n1, n2);

    EXPECT_EQ(buf[n1+0], (1000+0)/2);
    EXPECT_EQ(buf[n1+1], (1000+100)/2);

    rm(p);
}

TEST(Mix, NullAndZero) {
    std::string p="mix_null.wav";
    makewav(p, {1,2,3});
    WavReader r(p);
    Mix m(r,0);

    std::vector<int16_t> buf = {10,20,30};
    m.process(buf.data(), 0);
    m.process(nullptr, 5);

    EXPECT_EQ(buf[0],10);
    EXPECT_EQ(buf[1],20);
    EXPECT_EQ(buf[2],30);

    rm(p);
}

TEST(Mix, NameIsMix) {
    std::string p = "mix_name.wav";
    makewav(p, {1, 2, 3});

    WavReader r(p);
    Mix m(r, 0);

    EXPECT_STREQ(m.name(), "mix");

    rm(p);
}