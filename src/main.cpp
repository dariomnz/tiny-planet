#include <cstdlib>
#include <exception>
#include <iostream>

#include "Game.h"

int main() {
    try {
        Game game;
        game.run();
        // On web the Emscripten loop never returns; kept for clarity.
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
