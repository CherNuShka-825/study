#include "mix.h"

#include <string>
#include <vector>

#include "config_parser.h"
#include "audio_stream.h"
#include "converter_factory.h"


Mix::Mix(WavReader& source, int insertSec)
        : source_(source)
        , insertSample_(static_cast<std::size_t>(insertSec) * SAMPLE_RATE)
{}

void Mix::process(std::int16_t* buffer, std::size_t count) {
    if (!buffer || count == 0)
        return;

    std::size_t chunkStart = currentPos_;
    std::size_t chunkEnd   = currentPos_ + count;

    if (chunkEnd <= insertSample_) {
        currentPos_ += count;
        return;
    }

    std::size_t mixStart = (chunkStart < insertSample_)
                           ? insertSample_
                           : chunkStart;
    std::size_t mixCount = chunkEnd - mixStart;

    temp_.resize(mixCount);

    std::size_t got = source_.readSamples(temp_.data(), mixCount);

    for (std::size_t i = 0; i < count; ++i) {
        std::size_t globalPos = chunkStart + i;

        if (globalPos < insertSample_) {
            continue;
        }

        std::size_t j = globalPos - mixStart;

        if (j >= got) {
            continue;
        }

        int a = buffer[i];
        int b = temp_[j];
        buffer[i] = static_cast<std::int16_t>((a + b) / 2);
    }

    currentPos_ += count;
}

Converter* createMix(const ConfigCommand& cmd, const std::vector<WavReader*>& inputs) {
    if (cmd.args.size() != 1 && cmd.args.size() != 2) {
        throw ConverterError(
                "mix: expected 1 or 2 arguments: mix $N [insertSec]"
        );
    }

    const std::string& src = cmd.args[0];
    if (src.empty() || src[0] != '$') {
        throw ConverterError("mix: first argument must be $N");
    }

    char* end = nullptr;
    long idx1 = std::strtol(src.c_str() + 1, &end, 10);
    if (*end != '\0' || idx1 <= 0) {
        throw ConverterError("mix: invalid input index");
    }
    if (static_cast<std::size_t>(idx1) > inputs.size()) {
        throw ConverterError("mix: input index out of range");
    }

    int insertSec = 0;
    if (cmd.args.size() == 2) {
        long v = std::strtol(cmd.args[1].c_str(), &end, 10);
        if (*end != '\0' || v < 0) {
            throw ConverterError("mix: invalid insertSec");
        }
        insertSec = static_cast<int>(v);
    }

    return new Mix(*inputs[idx1 - 1], insertSec);
}

namespace {
    bool mix_reg = ConverterFactory::getInstance()->register_converter("mix", createMix);
}