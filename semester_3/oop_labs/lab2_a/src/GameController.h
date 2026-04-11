#ifndef LAB2_A_GAMECONTROLLER_H
#define LAB2_A_GAMECONTROLLER_H

#include <string>
#include "Universe.h"

class GameController {
private:
    Universe universe;
    bool running;

    void handlerCommand(const std::string& command);

    void commandDump(const std::string& filename);
    void commandTick(int n);
    void commandHelp();
public:
    GameController();
    GameController(const std::string& filename);

    void run();
};


#endif
