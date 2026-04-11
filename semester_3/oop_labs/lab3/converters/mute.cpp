#include "mute.h"

#include <algorithm>
#include <string>
#include <vector>

#include "config_parser.h"
#include "audio_stream.h"
#include "converter_factory.h"


Mute::Mute(int startSec, int endSec)
        : startSample_(static_cast<std::size_t>(startSec) * SAMPLE_RATE),
          endSample_(static_cast<std::size_t>(endSec)   * SAMPLE_RATE)
{}

void Mute::process(std::int16_t* buffer, std::size_t count) {
    if (!buffer || count == 0) return;

    std::size_t chunkStart = currentPos_;
    std::size_t chunkEnd   = currentPos_ + count;

    if (chunkEnd <= startSample_ || chunkStart >= endSample_) {
        currentPos_ += count;
        return;
    }

    std::size_t muteStart = std::max(chunkStart, startSample_);
    std::size_t muteEnd   = std::min(chunkEnd,   endSample_);

    std::size_t iStart = muteStart - chunkStart;
    std::size_t iEnd   = muteEnd   - chunkStart;

    for (std::size_t i = iStart; i < iEnd; ++i) {
        buffer[i] = 0;
    }

    currentPos_ += count;
}

Converter* createMute(const ConfigCommand& cmd, const std::vector<WavReader*>&) {
    if (cmd.args.size() != 2) {
        throw ConverterError("mute: expected 2 arguments");
    }
    int startSec = std::stoi(cmd.args[0]);
    int endSec   = std::stoi(cmd.args[1]);
    if (startSec < 0 || endSec < startSec) {
        throw ConverterError("mute: invalid time interval");
    }

    return new Mute(startSec, endSec);
}

namespace {
    bool mute_reg = ConverterFactory::getInstance()->register_converter("mute", createMute);
}
