#include "FileWork.h"
#include <fstream>
#include <sstream>
#include <iostream>


bool FileWork::loadFromFile(const std::string &filename, Universe &outUniverse) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return false;
    }

    std::string line;

    if (!std::getline(in, line)) {
        std::cerr << "Empty file: " << filename << "\n";
        return false;
    }
    if (line != "Life 1.06") {
        std::cerr << "Warning: first line is not 'Life 1.06' in file: " << filename << "\n";
    }

    std::string name = "Default";
    std::string ruleString = "B3/S23";
    std::vector<std::pair<int, int>> aliveCells;

    while (std::getline(in, line)) {
        if (line.empty()) {
            std::cerr << "Warning: empty line\n";
            continue;
        }

        if (line.rfind("#N", 0) == 0) {
            if (line.size() > 3) {
                name = line.substr(3);
            } else {
                std::cerr << "Warning: empty name after #N\n";
            }
            continue;
        }

        if (line.rfind("#R", 0) == 0) {
            if (line.size() > 3) {
                ruleString = line.substr(3);
            } else {
                std::cerr << "Warning: empty rule after #R\n";
            }
            continue;
        }

        int x, y;
        std::istringstream iss(line);
        if (!(iss >> x >> y)) {
            std::cerr << "Warning: cannot parse coordinate line: " << line << "\n";
            continue;
        }
        aliveCells.emplace_back(x, y);
    }

    Rule rule(ruleString);

    int maxX = -1, maxY = -1;
    for (auto &[x, y]: aliveCells) {
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
    }

    int width  = (maxX >= 0) ? maxX + 1 : 20;
    int height = (maxY >= 0) ? maxY + 1 : 20;

    Universe universe(width, height, rule, name);

    for (auto &[x, y] : aliveCells) {
        if (!universe.checkPosition(x, y)) {
            std::cerr << "Warning: cell (" << x << ", " << y << ") is out of bounds. Ignored.\n";
            continue;
        }
        universe.setAlive(x, y, true);
    }

    outUniverse = std::move(universe);
    return true;
}


bool FileWork::saveToFile(const std::string &filename, const Universe &universe) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Error: cannot open file for writing: " << filename << "\n";
        return false;
    }
    out << "Life 1.06\n";
    out << "#N " << universe.getName() << "\n";
    out << "#R " << universe.getRule().toString() << "\n";

    int width = universe.getWidth();
    int height = universe.getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (universe.isAlive(x, y)) {
                out << x << " " << y << "\n";
            }
        }
    }
    return true;
}