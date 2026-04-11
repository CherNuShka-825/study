#ifndef LAB2_A_UNIVERSE_H
#define LAB2_A_UNIVERSE_H

#include <string>
#include <vector>
#include "Rule.h"

class Universe {
private:
    std::string name;
    int width, height;
    std::vector<std::vector<bool>> matUniverse;
    Rule rule;
    int numTick;

    bool nextCellState(int x, int y) const;
    int countNeighbors(int x, int y) const;

public:
    Universe();
    Universe(int w, int h, const Rule& r, const std::string& n = "");

    void setName(const std::string& n);
    const std::string& getName() const;

    void setRule(const Rule& r);
    const Rule& getRule() const;

    bool checkPosition(int x, int y) const;

    void setAlive(int x, int y, bool alive = true);
    bool isAlive(int x, int y) const;

    int getWidth() const;
    int getHeight() const;
    int getTick() const;

    void tick();
    void tick(int n);

    std::string toString() const;
};


#endif
