#include "nitro-studio/application.h"
#include <cstdio>
int main()
{

    try
    {
        nitro::studio::Application app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}