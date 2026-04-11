#include "GameController.h"
#include "FileWork.h"
#include <iostream>
#include <sstream>

GameController::GameController()
    : universe(),
      running(true)
{
    std::cout << universe.toString() << "\n";
}

GameController::GameController(const std::string &filename)
    : universe(),
      running(true)
{
    Universe loaded;
    if (FileWork::loadFromFile(filename, loaded)) {
        universe = std::move(loaded);
        std::cout << "Loaded universe from file: " << filename << "\n";
    } else {
        std::cerr << "Failed to load universe from file. Using default universe.\n";
    }
    std::cout << universe.toString() << "\n";
}

void GameController::run() {
    std::string line;
    while (running) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput ended. Exiting.\n";
            break;
        }
        if (line.empty()) {
            continue;
        }
        handlerCommand(line);
    }
}

void GameController::handlerCommand(const std::string &command) {
    std::istringstream iss(command);
    std::string  keyword;
    iss >> keyword;
    if (keyword.empty()) return;

    if (keyword == "help") {
        commandHelp();
    } else if (keyword == "exit") {
        running = false;
        std::cout << "Exiting.\n";
    } else if (keyword == "dump") {
        std::string filename;
        iss >> filename;
        if (filename.empty()) {
            std::cerr << "Usage: dump <filename>\n";
            return;
        }
        commandDump(filename);
    } else if (keyword == "tick" || keyword == "t") {
        int n = 1;
        if (iss >> n) {
            if (n <= 0) {
                std::cerr << "Tick count must be positive.\n";
                return;
            }
        }
        commandTick(n);
    } else {
        std::cerr << "Unknown command: " << keyword << "\n";
        std::cerr << "Type 'help' for available commands.\n";
    }
}

void GameController::commandDump(const std::string &filename) {
    if (FileWork::saveToFile(filename, universe)) {
        std::cout << "Universe saved to file: " << filename << "\n";
    } else {
        std::cerr << "Failed to save universe to file: " << filename << "\n";
    }
}

void GameController::commandTick(int n) {
    universe.tick(n);
    std::cout << "universe: " << universe.getName()
              << "; Rule: " << universe.getRule().toString()
              << "; Tick: " << universe.getTick() << "\n";
    std::cout << universe.toString() << "\n";
}

void GameController::commandHelp() {
    std::cout << "Available commands:\n";
    std::cout << "  help               - show this help\n";
    std::cout << "  tick [n] / t [n]   - advance simulation by n steps (default 1)\n";
    std::cout << "  dump <filename>    - save current universe to file\n";
    std::cout << "  exit               - quit\n";
}
