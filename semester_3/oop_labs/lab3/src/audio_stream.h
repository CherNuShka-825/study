#pragma once

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <string>
#include <stdexcept>


struct WavInfo {
    std::uint32_t sampleRate    = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint16_t numChannels   = 0;
    std::uint32_t dataSizeBytes = 0;
    std::uint32_t totalSamples  = 0;
};

class WavReaderError : public std::runtime_error {
public:
    explicit WavReaderError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class WavWriterError : public std::runtime_error {
public:
    explicit WavWriterError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class WavFile {
public:
    const WavInfo& info() const noexcept {
        return info_;
    }

protected:
    WavFile() = default;
    WavInfo info_{};
};

class WavReader : public WavFile {
public:
    explicit WavReader(const std::string& fileName);
    std::size_t readSamples(std::int16_t* buffer, std::size_t maxSamples);
    bool eof() const noexcept { return dataBytesLeft_ == 0; }

private:
    std::ifstream file_;
    std::streampos dataStart_ = 0;
    std::uint32_t dataBytesLeft_ = 0;

    void parseHeader();
};

class WavWriter : public WavFile {
public:
    explicit WavWriter(const std::string& fileName, const WavInfo& info);
    ~WavWriter();
    void writeSamples(const std::int16_t* buffer, std::size_t sampleCount);
    void close() noexcept;

private:
    std::ofstream file_;
    std::uint32_t dataBytesWritten_ = 0;
    bool finalized_ = false;

    void writeHeaderPlaceholder();
    void finalizeHeader();
};
