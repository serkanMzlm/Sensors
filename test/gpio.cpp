#include <iostream>
#include <unistd.h> 

#include "gpio_control.hpp"

#define DELAY 100000

int main(int argc, char** argv){
    std::cout << "Toplam argüman sayısı: " << argc << std::endl;
    std::cout << "Program adı: " << argv[0] << std::endl;
    if (argc > 2) {
        int pin = atoi(argv[1]);
        int value = atoi(argv[2]);

        unexportPin(pin);
        usleep(DELAY);
        exportPin(pin);
        usleep(DELAY);
        directionPin(pin, IN);
        usleep(DELAY);
        std::cout << read(pin);
    }

    return 0;
}