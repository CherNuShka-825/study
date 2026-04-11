#include <windows.h>
#include <iostream>
#include "GameController.h"
#include "FileWork.h"

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    bool haveIterations = false;
    bool haveOutput = false;
    int iterations = 0;
    std::string inputFile;
    std::string outputFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--iterations=", 0) == 0) {
            std::string numString = arg.substr(std::string("--iterations=").size());
            try {
                iterations = std::stoi(numString);
                haveIterations = true;
            } catch (...) {
                std::cerr << "Invalid value for --iterations: " << numString << "\n";
                return 1;
            }
        } else if (arg == "-i") {
            if (i + 1 >= argc) {
                std::cerr << "Option -i requires a number\n";
                return 1;
            }
            std::string numString = argv[++i];
            try {
                iterations = std::stoi(numString);
                haveIterations = true;
            } catch (...) {
                std::cerr << "Invalid value for -i: " << numString << "\n";
                return 1;
            }
        } else if (arg.rfind("--output=", 0) == 0) {
            outputFile = arg.substr(std::string("--output=").size());
            haveOutput = true;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Option -o requires a filename\n";
                return 1;
            }
            outputFile = argv[++i];
            haveOutput = true;
        } else if (!arg.empty() && arg[0] != '-') {
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "Warning: extra argument ignored: " << arg << "\n";
            }
        } else {
            std::cerr << "Warning: unknown option: " << arg << "\n";
        }
    }

    if (haveIterations) {
        if (inputFile.empty()) {
            std::cerr << "Offline mode: need input file.\n";
            std::cerr << "Usage: program <input> -i N -o <output>\n";
            return 1;
        }
        if (iterations <= 0) {
            std::cerr << "Iterations must be positive.\n";
            return 1;
        }
        if (!haveOutput) {
            outputFile = "output.txt";
            std::cerr << "No output file specified. Using default: " << outputFile << "\n";
        }
        Universe universe;
        if (!FileWork::loadFromFile(inputFile, universe)) {
            std::cerr << "Failed to load universe from file: " << inputFile << "\n";
            return 1;
        }
        universe.tick(iterations);
        if (!FileWork::saveToFile(outputFile, universe)) {
            std::cerr << "Failed to save universe to file: " << outputFile << "\n";
            return 1;
        }
        std::cout << "Offline mode: " << iterations << " iterations done.\n";
        std::cout << "Result saved to: " << outputFile << "\n";
        return 0;
    }

    if (!inputFile.empty()) {
        GameController controller(inputFile);
        controller.run();
    } else {
        GameController controller;
        controller.run();
    }
    return 0;
}
