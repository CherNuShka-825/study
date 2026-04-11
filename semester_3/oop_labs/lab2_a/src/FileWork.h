#ifndef LAB2_A_FILEWORK_H
#define LAB2_A_FILEWORK_H

#include <string>
#include "Universe.h"

class FileWork {
public:
    static bool loadFromFile(const std::string& filename, Universe& outUniverse);
    static bool saveToFile(const std::string& filename, const Universe& universe);
};

#endif
