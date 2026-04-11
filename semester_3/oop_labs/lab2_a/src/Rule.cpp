#include "Rule.h"
#include <iostream>


Rule::Rule() {
    for (int i = 0; i <= 8; ++i) {
        birth[i] = false;
        survive[i] = false;
    }

    birth[3] = true;
    survive[2] = true;
    survive[3] = true;
}

Rule::Rule(const std::string &ruleString) {
    for (int i = 0; i <= 8; ++i) {
        birth[i] = false;
        survive[i] = false;
    }
    parse(ruleString);
}

bool Rule::isBirth(int neighbors) const {
    return birth[neighbors];
}

bool Rule::isSurvive(int neighbors) const {
    return survive[neighbors];
}

std::string Rule::toString() const{
    std::string b = "B";
    std::string s = "S";
    for (int i = 0; i < 9; i++) {
        if (birth[i]) b += std::to_string(i);
        if (survive[i]) s += std::to_string(i);
    }
    return b + "/" + s;
}

void Rule::parse(const std::string &ruleString) {
    size_t bPos = ruleString.find('B');
    size_t sPos = ruleString.find('S');

    if (bPos == std::string::npos || sPos == std::string::npos) {
        std::cerr << "Invalid rule format\n";
        return;
    }

    for (int i = 0; i <= 8; ++i) {
        birth[i] = false;
        survive[i] = false;
    }

    for (size_t i = bPos + 1; i < sPos; i++) {
        char c = ruleString[i];
        if (c >= '0' && c <= '8') {
            birth[c - '0'] = true;
        } else {
            std::cerr << "Invalid rule format\n";
        }
    }

    for (size_t i = sPos + 1; i < ruleString.size(); ++i) {
        char c = ruleString[i];
        if (c >= '0' && c <= '8') {
            survive[c - '0'] = true;
        } else {
            std::cerr << "Invalid rule format\n";
        }
    }
}