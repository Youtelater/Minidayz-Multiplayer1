#include "minidayz/engine.hpp"

#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    minidayz::Engine engine;
    if (!engine.initialize()) {
        std::printf("Mini DAYZ+: engine initialization failed.\n");
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
