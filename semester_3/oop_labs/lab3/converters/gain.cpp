#include "gain.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "config_parser.h"
#include "audio_stream.h"
#include "converter_factory.h"


Gain::Gain(int startSec, int endSec, float factor)
        : startSample_(static_cast<std::size_t>(startSec) * SAMPLE_RATE),
          endSample_(static_cast<std::size_t>(endSec)   * SAMPLE_RATE),
          factor_(factor)
{}

void Gain::process(std::int16_t* buffer, std::size_t count) {
    if (!buffer || count == 0) return;

    std::size_t chunkStart = currentPos_;
    std::size_t chunkEnd   = currentPos_ + count;

    if (chunkEnd <= startSample_ || chunkStart >= endSample_) {
        currentPos_ += count;
        return;
    }

    std::size_t gainStart = std::max(chunkStart, startSample_);
    std::size_t gainEnd   = std::min(chunkEnd,   endSample_);

    std::size_t iStart = gainStart - chunkStart;
    std::size_t iEnd   = gainEnd   - chunkStart;

    for (std::size_t i = iStart; i < iEnd; ++i) {
        int scaled = static_cast<int>(buffer[i] * factor_);

        if (scaled > 32767) {
            scaled = 32767;
        } else if (scaled < -32768) {
            scaled = -32768;
        }

        buffer[i] = static_cast<std::int16_t>(scaled);
    }

    currentPos_ += count;
}

Converter* createGain(const ConfigCommand& cmd, const std::vector<WavReader*>&) {
    if (cmd.args.size() != 3) {
        throw ConverterError("gain: expected 3 arguments: gain startSec endSec factor");
    }

    char* end = nullptr;
    long startSec = std::strtol(cmd.args[0].c_str(), &end, 10);
    if (*end != '\0') {
        throw ConverterError("gain: invalid startSec");
    }

    end = nullptr;
    long endSec = std::strtol(cmd.args[1].c_str(), &end, 10);
    if (*end != '\0') {
        throw ConverterError("gain: invalid endSec");
    }

    end = nullptr;
    double factor = std::strtod(cmd.args[2].c_str(), &end);
    if (*end != '\0') {
        throw ConverterError("gain: invalid factor");
    }

    if (startSec < 0 || endSec < startSec) {
        throw ConverterError("gain: bad interval");
    }

    return new Gain(static_cast<int>(startSec),
                    static_cast<int>(endSec),
                    static_cast<float>(factor));
}

namespace {
    bool gain_reg = ConverterFactory::getInstance()->register_converter("gain", createGain);
}