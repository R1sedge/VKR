#include <iostream>
#include <windows.h>
#include "render/renderer.h"

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    Renderer renderer = Renderer(1280, 720);

    renderer.mainLoop();
    
    std::cout << "main() finished";
    return 0;
}
