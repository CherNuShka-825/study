#include <gtest/gtest.h>

#include "audio_stream.h"

#include <vector>
#include <cstdio>
#include <fstream>
#include <string>


static void removeFileIfExists(const std::string& path) {
    std::remove(path.c_str());
}


TEST(AudioStream, WriteThenReadBack) {
    const std::string fileName = "test_io.wav";
    removeFileIfExists(fileName);

    std::vector<std::int16_t> samples = {
            0,
            1000,
            -1000,
            32767,
            -32768 + 1,
            12345,
            -23456
    };

    WavInfo info;
    info.sampleRate    = 44100;
    info.bitsPerSample = 16;
    info.numChannels   = 1;
    info.dataSizeBytes = static_cast<std::uint32_t>(samples.size() * 2);
    info.totalSamples  = static_cast<std::uint32_t>(samples.size());

    {
        WavWriter writer(fileName, info);
        writer.writeSamples(samples.data(), samples.size());
    }

    WavReader reader(fileName);
    const WavInfo& readInfo = reader.info();

    EXPECT_EQ(readInfo.sampleRate,    info.sampleRate);
    EXPECT_EQ(readInfo.bitsPerSample, info.bitsPerSample);
    EXPECT_EQ(readInfo.numChannels,   info.numChannels);
    EXPECT_EQ(readInfo.dataSizeBytes, info.dataSizeBytes);
    EXPECT_EQ(readInfo.totalSamples,  info.totalSamples);

    std::vector<std::int16_t> readBuf(samples.size());
    std::size_t readCount = reader.readSamples(readBuf.data(), readBuf.size());

    EXPECT_EQ(readCount, samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        EXPECT_EQ(readBuf[i], samples[i]) << "Sample mismatch at index " << i;
    }

    std::size_t readCount2 = reader.readSamples(readBuf.data(), readBuf.size());
    EXPECT_EQ(readCount2, 0u);
    EXPECT_TRUE(reader.eof());

    removeFileIfExists(fileName);
}


TEST(AudioStream, ReaderThrowsIfFileNotFound) {
    std::string fileName = "this_file_should_not_exist_123456.wav";
    removeFileIfExists(fileName);

    EXPECT_THROW(
            {
                WavReader reader(fileName);
            },
            WavReaderError
    );
}


TEST(AudioStream, ReaderThrowsOnInvalidRiff) {
    const std::string fileName = "invalid_riff.wav";
    removeFileIfExists(fileName);

    {
        std::ofstream f(fileName.c_str(), std::ios::binary);
        f << "HELLO WORLD";
    }

    EXPECT_THROW(
            {
                WavReader reader(fileName);
            },
            WavReaderError
    );

    removeFileIfExists(fileName);
}


TEST(AudioStream, WriterThrowsOnUnsupportedFormat) {
    WavInfo badInfo = {};
    badInfo.sampleRate    = 48000;
    badInfo.bitsPerSample = 16;
    badInfo.numChannels   = 1;

    EXPECT_THROW(
            {
                WavWriter writer("dummy.wav", badInfo);
            },
            WavWriterError
    );

    WavInfo badInfo2 = {};
    badInfo2.sampleRate    = 44100;
    badInfo2.bitsPerSample = 8;
    badInfo2.numChannels   = 1;

    EXPECT_THROW(
            {
                WavWriter writer("dummy2.wav", badInfo2);
            },
            WavWriterError
    );
}


TEST(AudioStream, ReadInChunks) {
    const std::string fileName = "test_chunks.wav";
    removeFileIfExists(fileName);

    std::vector<std::int16_t> samples(10);
    for (int i = 0; i < 10; ++i) {
        samples[i] = static_cast<std::int16_t>(i * 1000 - 5000);
    }

    WavInfo info;
    info.sampleRate    = 44100;
    info.bitsPerSample = 16;
    info.numChannels   = 1;
    info.dataSizeBytes = static_cast<std::uint32_t>(samples.size() * 2);
    info.totalSamples  = static_cast<std::uint32_t>(samples.size());

    {
        WavWriter writer(fileName, info);
        writer.writeSamples(samples.data(), samples.size());
    }

    WavReader reader(fileName);
    std::vector<std::int16_t> buf(3);
    std::vector<std::int16_t> allRead;

    while (true) {
        std::size_t n = reader.readSamples(buf.data(), buf.size());
        if (n == 0) break;
        allRead.insert(allRead.end(), buf.begin(), buf.begin() + static_cast<long>(n));
    }

    EXPECT_EQ(allRead.size(), samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        EXPECT_EQ(allRead[i], samples[i]) << "Sample mismatch at index " << i;
    }

    EXPECT_TRUE(reader.eof());

    removeFileIfExists(fileName);
}
