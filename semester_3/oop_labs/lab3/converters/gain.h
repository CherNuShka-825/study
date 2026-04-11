#pragma once

#include "converter.h"
#include <cstddef>
#include <cstdint>

class Gain : public Converter {
public:
    Gain(int startSec, int endSec, float factor);

    const char* name() const noexcept override { return "gain"; }
    void process(std::int16_t* buffer, std::size_t count) override;

private:
    static constexpr std::size_t SAMPLE_RATE = 44100;

    std::size_t startSample_;
    std::size_t endSample_;
    float       factor_ = 1.0f;
    std::size_t currentPos_ = 0;
};
