#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "converter.h"
#include "audio_stream.h"

class Mix : public Converter {
public:
    Mix(WavReader& source, int insertSec);

    const char* name() const noexcept override { return "mix"; }
    void process(std::int16_t* buffer, std::size_t count) override;

private:
    static constexpr std::size_t SAMPLE_RATE = 44100;

    WavReader& source_;
    std::size_t insertSample_;
    std::size_t currentPos_ = 0;

    std::vector<std::int16_t> temp_;
};
