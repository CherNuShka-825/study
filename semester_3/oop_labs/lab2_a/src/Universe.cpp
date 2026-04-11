#include "Universe.h"
#include <iostream>

namespace {
    int mod(int a, int m) {
        return (a % m + m) % m;
    }
}

Universe::Universe()
    :Universe(20, 20, Rule(), "Default")
{
    const std::pair<int,int> pulsar[] = {
            {4, 4}, {5, 5}, {6, 3}, {6, 4}, {6, 5}
    };
    for (auto [x, y] : pulsar) {
        setAlive(x, y);
    }
}

Universe::Universe(int w, int h, const Rule& r, const std::string &n)
    : width(w),
      height(h),
      rule(r),
      name(n),
      numTick(0),
      matUniverse(h, std::vector<bool>(w, false))
{
}

void Universe::setName(const std::string &n) {
    name = n;
}

const std::string &Universe::getName() const {
    return name;
}

void Universe::setRule(const Rule& r) {
    rule = r;
}

const Rule &Universe::getRule() const {
    return rule;
}

bool Universe::checkPosition(int x, int y) const {
    if (x < 0 || x >= width) {
        std::cerr << "Invalid x: " << x << "\n";
        return false;
    }
    if (y < 0 || y >= height) {
        std::cerr << "Invalid x: " << y << "\n";
        return false;
    }
    return true;
}

void Universe::setAlive(int x, int y, bool alive) {
    if (!checkPosition(x, y)) {
        return;
    }
    matUniverse[y][x] = alive;
}

bool Universe::isAlive(int x, int y) const {
    if (!checkPosition(x, y)) {
        return false;
    }
    return matUniverse[y][x];
}

int Universe::getWidth() const {
    return width;
}

int Universe::getHeight() const {
    return height;
}

int Universe::getTick() const {
    return numTick;
}

int Universe::countNeighbors(int x, int y) const {
    int res = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dy == 0 && dx == 0)
                continue;
            if (matUniverse[mod(y + dy, height)][mod(x + dx, width)]) {
                ++res;
            }
        }
    }
    return res;
}

bool Universe::nextCellState(int x, int y) const {
    int neighbors = countNeighbors(x, y);
    if (isAlive(x, y)) {
        return rule.isSurvive(neighbors);
    }
    return rule.isBirth(neighbors);
}

void Universe::tick() {
    std::vector<std::vector<bool>> newMatUniverse(
            height, std::vector<bool>(width, false)
    );

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            newMatUniverse[y][x] = nextCellState(x, y);
        }
    }
    matUniverse = std::move(newMatUniverse);
    ++numTick;
}

void Universe::tick(int n) {
    if (n < 0) {
        std::cerr << "Invalid number ticks\n";
        return;
    }
    for (int i = 0; i < n; ++i) {
        tick();
    }
}

std::string Universe::toString() const {
    std::string res = "";
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (matUniverse[y][x]) {
                res += "⬜";
            } else {
                res += "⬛";
            }
        }
        if (y + 1 < height) res += "\n";
    }
    return res;
}
