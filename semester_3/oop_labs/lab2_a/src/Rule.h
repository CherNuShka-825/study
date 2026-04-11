#ifndef LAB2_A_RULE_H
#define LAB2_A_RULE_H

#include <string>

class Rule {
private:
    bool birth[9];
    bool survive[9];

public:
    Rule();
    Rule(const std::string& ruleString);

    bool isBirth(int neighbors) const;
    bool isSurvive(int neighbors) const;

    std::string toString() const;
    void parse(const std::string& ruleString);
};

#endif
