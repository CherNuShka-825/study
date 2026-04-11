#include "audio_stream.h"
#include <cstring>
#include <algorithm>


WavReader::WavReader(const std::string &fileName)
    : file_(fileName.c_str(), std::ios::binary)
{
    if (!file_) {
        throw WavReaderError("cannot open WAV file: " + fileName);
    }
    parseHeader();
}

void WavReader::parseHeader() {
    char riffId[4];
    file_.read(riffId, 4);
    if (!file_ || std::strncmp(riffId, "RIFF", 4) != 0) {
        throw WavReaderError("invalid WAV file: no RIFF");
    }

    std::uint32_t riffSize = 0;
    file_.read(reinterpret_cast<char*>(&riffSize), 4);
    if (!file_) {
        throw WavReaderError("invalid WAV file: cannot read RIFF size");
    }

    char waveId[4];
    file_.read(waveId, 4);
    if (!file_ || std::strncmp(waveId, "WAVE", 4) != 0) {
        throw WavReaderError("invalid WAV file: not WAVE");
    }

    bool fmtFound = false;
    bool dataFound = false;

    std::uint16_t audioFormat   = 0;
    std::uint16_t numChannels   = 0;
    std::uint32_t sampleRate    = 0;
    std::uint32_t byteRate      = 0;
    std::uint16_t blockAlign    = 0;
    std::uint16_t bitsPerSample = 0;

    std::uint32_t dataSize = 0;

    while (file_) {
        char chunkId[4];
        file_.read(chunkId, 4);
        if (!file_) break;

        std::uint32_t chunkSize = 0;
        file_.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file_) {
            throw WavReaderError("invalid WAV file: cannot read chunk size");
        }

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {

            if (chunkSize < 16) {
                throw WavReaderError("invalid WAV: fmt chunk too small");
            }

            file_.read(reinterpret_cast<char*>(&audioFormat),   2);
            file_.read(reinterpret_cast<char*>(&numChannels),   2);
            file_.read(reinterpret_cast<char*>(&sampleRate),    4);
            file_.read(reinterpret_cast<char*>(&byteRate),      4);
            file_.read(reinterpret_cast<char*>(&blockAlign),    2);
            file_.read(reinterpret_cast<char*>(&bitsPerSample), 2);

            if (!file_) {
                throw WavReaderError("invalid WAV: cannot read fmt body");
            }

            if (chunkSize > 16) {
                file_.seekg(chunkSize - 16, std::ios::cur);
                if (!file_) {
                    throw WavReaderError("invalid WAV: cannot skip extra fmt data");
                }
            }
            fmtFound = true;
        }
        else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataSize     = chunkSize;
            dataStart_   = file_.tellg();
            dataBytesLeft_ = dataSize;
            dataFound    = true;
            break;
        }
        else {
            file_.seekg(chunkSize, std::ios::cur);
            if (!file_) {
                throw WavReaderError("invalid WAV: cannot skip unknown chunk");
            }
        }
    }
    if (!fmtFound) {
        throw WavReaderError("invalid WAV: no fmt chunk");
    }
    if (!dataFound) {
        throw WavReaderError("invalid WAV: no data chunk");
    }

    if (audioFormat != 1) {
        throw WavReaderError("unsupported WAV: audioFormat != PCM");
    }
    if (numChannels != 1) {
        throw WavReaderError("unsupported WAV: numChannels != 1 (mono)");
    }
    if (sampleRate != 44100) {
        throw WavReaderError("unsupported WAV: sampleRate != 44100");
    }
    if (bitsPerSample != 16) {
        throw WavReaderError("unsupported WAV: bitsPerSample != 16");
    }
    if (blockAlign != 2) {
        throw WavReaderError("unsupported WAV: blockAlign != 2");
    }

    std::uint32_t expectedByteRate = sampleRate * blockAlign;
    if (byteRate != expectedByteRate) {
        throw WavReaderError("unsupported WAV: invalid byteRate");
    }

    info_.sampleRate    = sampleRate;
    info_.bitsPerSample = bitsPerSample;
    info_.numChannels   = numChannels;
    info_.dataSizeBytes = dataSize;
    info_.totalSamples  = dataSize / 2;
}

std::size_t WavReader::readSamples(std::int16_t* buffer, std::size_t maxSamples)
{
    if (!buffer || maxSamples == 0) {
        return 0;
    }
    if (dataBytesLeft_ == 0) {
        return 0;
    }

    std::uint32_t bytesToRead = static_cast<std::uint32_t>(
            std::min<std::size_t>(maxSamples * 2, dataBytesLeft_)
    );

    file_.read(reinterpret_cast<char*>(buffer), bytesToRead);

    std::streamsize actuallyRead = file_.gcount();
    if (actuallyRead <= 0) {
        dataBytesLeft_ = 0;
        return 0;
    }

    dataBytesLeft_ -= static_cast<std::uint32_t>(actuallyRead);

    return static_cast<std::size_t>(actuallyRead) / 2;
}



WavWriter::WavWriter(const std::string& fileName, const WavInfo& info)
        : file_(fileName.c_str(), std::ios::binary)
{
    if (!file_) {
        throw WavWriterError("cannot open WAV file for writing: " + fileName);
    }

    info_ = info;

    if (info_.sampleRate != 44100 ||
        info_.bitsPerSample != 16 ||
        info_.numChannels != 1)
    {
        throw WavWriterError("WavWriter supports only PCM 44100 Hz, 16-bit, mono");
    }

    writeHeaderPlaceholder();
}

WavWriter::~WavWriter() {
    try {
        close();
    } catch (...) {
    }
}

void WavWriter::close() noexcept {
    if (finalized_) {
        return;
    }

    try {
        finalizeHeader();
    } catch (...) {
    }

    finalized_ = true;
}

void WavWriter::writeHeaderPlaceholder() {
    file_.write("RIFF", 4);

    std::uint32_t riffSize = 0;
    file_.write(reinterpret_cast<const char*>(&riffSize), 4);

    file_.write("WAVE", 4);

    file_.write("fmt ", 4);

    std::uint32_t fmtSize = 16;
    file_.write(reinterpret_cast<const char*>(&fmtSize), 4);

    // fmt-data (16 bytes):
    std::uint16_t audioFormat = 1; // PCM
    file_.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file_.write(reinterpret_cast<const char*>(&info_.numChannels), 2);
    file_.write(reinterpret_cast<const char*>(&info_.sampleRate), 4);

    std::uint16_t blockAlign = static_cast<std::uint16_t>(
            info_.numChannels * (info_.bitsPerSample / 8)
    );
    std::uint32_t byteRate = info_.sampleRate * blockAlign;

    file_.write(reinterpret_cast<const char*>(&byteRate), 4);
    file_.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file_.write(reinterpret_cast<const char*>(&info_.bitsPerSample), 2);

    file_.write("data", 4);

    std::uint32_t dataSize = 0;
    file_.write(reinterpret_cast<const char*>(&dataSize), 4);

    if (!file_) {
        throw WavWriterError("failed to write WAV header placeholder");
    }
}

void WavWriter::finalizeHeader() {
    // 0  - "RIFF"
    // 4  - riffSize
    // 8  - "WAVE"
    // 12 - "fmt "
    // 16 - fmtSize
    // 20 - fmt body (16 байт)
    // 36 - "data"
    // 40 - dataSize (end 44)
    if (!file_) return;

    std::uint32_t dataSize = dataBytesWritten_;

    // riffSize = 4 (WAVE)
    //          + (8 + 16)  fmt
    //          + (8 + dataSize)
    // Res: 36 + dataSize
    std::uint32_t riffSize = 36 + dataSize;

    file_.seekp(4, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(&riffSize), 4);

    file_.seekp(40, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(&dataSize), 4);

    file_.flush();
}

void WavWriter::writeSamples(const std::int16_t* buffer, std::size_t sampleCount) {
    if (!file_) {
        throw WavWriterError("WAV file is not writable");
    }
    if (!buffer || sampleCount == 0) {
        return;
    }

    // 2 bytes = 1 sample
    std::uint32_t bytesToWrite = static_cast<std::uint32_t>(sampleCount) * 2u;

    file_.write(reinterpret_cast<const char*>(buffer), bytesToWrite);

    dataBytesWritten_ += bytesToWrite;
}
