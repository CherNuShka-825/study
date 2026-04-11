#include <iostream>
#include "module1.h"
#include "module2.h"

int main() {
    std::cout << "Hello world!" << std::endl;

    std::cout << Module1::getMyName() << std::endl;
    std::cout << Module2::getMyName() << std::endl;

    using namespace Module1;
    std::cout << getMyName() << std::endl;
    std::cout << Module2::getMyName() << std::endl;

//    using namespace module2;
//    std::cout << getmyname() << std::endl;

    using Module2::getMyName;
    std::cout << getMyName() << std::endl;
    return 0;
}
