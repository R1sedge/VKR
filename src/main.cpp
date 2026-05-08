#include <iostream>
#include <windows.h>
#include "app/app.h"

int main(int argc, char** argv) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    App app;
    if (!app.initialize(argc, argv))
    {
        std::cerr << "Не получилось инициализировать App\n";
        return EXIT_FAILURE;
    }

    app.run();
    app.shutDown();

    return EXIT_SUCCESS;
}
