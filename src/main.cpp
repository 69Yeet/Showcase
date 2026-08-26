#include "renderer.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char **argv) 
{
    Renderer app;

    app.setMainArguments(argc, argv);
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
