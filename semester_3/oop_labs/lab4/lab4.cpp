#include <fstream>
#include <iostream>
#include <filesystem>
#include "csv_parser.h"

int main() {
    std::cout << "cwd = " << std::filesystem::current_path() << "\n";

    std::ifstream file("test.csv");
    if (!file.is_open()) {
        std::cout << "FAILED to open test.csv\n";
        return 0;
    }

    // У тебя 4 колонки, разделитель ';' у нас уже по умолчанию
    CSVParser<std::string, std::string, std::string, std::string> parser(file, 0);

    try {
        for (auto row : parser) {
            std::cout << row << "\n";
        }
    }
    catch (const CSVParseError& e) {
        std::cout << e.what() << "\n";
    }
}
