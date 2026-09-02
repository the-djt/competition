#include "app/game_app.hpp"

#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    std::string screenshot;
    std::string qaScene = "battle";
    bool smoke = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--qa-screenshot" && i + 1 < argc) screenshot = argv[++i];
        if (argument == "--smoke") smoke = true;
    }
    if (const char* qaPath = std::getenv("BFB_QA_SCREENSHOT")) screenshot = qaPath;
    if (const char* scene = std::getenv("BFB_QA_SCENE")) qaScene = scene;
    bfb::GameApp app;
    return app.run(screenshot, smoke, qaScene);
}
