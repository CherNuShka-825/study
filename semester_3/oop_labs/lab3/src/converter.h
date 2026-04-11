#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>


class ConverterError : public std::runtime_error {
public:
    explicit ConverterError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class Converter {
public:
    virtual ~Converter() = default;
    virtual void process(std::int16_t* buffer, std::size_t count) = 0;
    virtual const char* name() const noexcept = 0;
};
