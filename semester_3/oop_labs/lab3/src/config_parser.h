#pragma once

#include <string>
#include <vector>
#include <stdexcept>

struct ConfigCommand {
    std::string name;
    std::vector<std::string> args;
    std::size_t line = 0;
};

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class ConfigParser {
public:
    explicit ConfigParser(std::string fileName);
    std::vector<ConfigCommand> parse();

private:
    std::string fileName_;
};
