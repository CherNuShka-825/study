#include "config_parser.h"

#include <fstream>
#include <sstream>
#include <cctype>

namespace {

    static void ltrim(std::string& s) {
        std::size_t i = 0;
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
        s.erase(0, i);
    }

    static void rtrim(std::string& s) {
        if (s.empty()) return;
        std::size_t i = s.size();
        while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) {
            --i;
        }
        s.erase(i);
    }

    static void trim(std::string& s) {
        rtrim(s);
        ltrim(s);
    }

    static std::vector<std::string> split_tokens(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

}


ConfigParser::ConfigParser(std::string fileName)
        : fileName_(std::move(fileName))
{
}

std::vector<ConfigCommand> ConfigParser::parse() {
    std::ifstream in(fileName_);
    if (!in) {
        throw ConfigError("cannot open config file: " + fileName_);
    }

    std::vector<ConfigCommand> result;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(in, line)) {
        ++lineNumber;

        trim(line);
        if (line.empty()) {
            continue;
        }

        if (!line.empty() && line[0] == '#') {
            continue;
        }

        auto tokens = split_tokens(line);

        ConfigCommand cmd;
        cmd.line = lineNumber;
        cmd.name = std::move(tokens[0]);
        cmd.args.reserve(tokens.size() - 1);

        for (std::size_t i = 1; i < tokens.size(); ++i) {
            cmd.args.push_back(std::move(tokens[i]));
        }

        result.push_back(std::move(cmd));
    }

    return result;
}
