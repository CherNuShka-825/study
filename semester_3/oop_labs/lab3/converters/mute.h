#pragma once

#include "converter.h"
#include <cstddef>
#include <cstdint>


class Mute : public Converter {
public:
    Mute(int startSec, int endSec);

    const char* name() const noexcept override { return "mute"; }
    void process(std::int16_t* buffer, std::size_t count) override;

private:
    static constexpr int SAMPLE_RATE = 44100;

    std::size_t startSample_;
    std::size_t endSample_;
    std::size_t currentPos_ = 0;
};
